#include "models.hpp"

#include <algorithm>
#include <iomanip>
#include <regex>
#include <sstream>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/result_set.hpp>

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) noexcept {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

std::string Fnv1aHex(std::string_view value) {
    constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t kPrime        = 1099511628211ULL;
    std::uint64_t hash = kOffsetBasis;
    for (unsigned char ch : value) {
        hash ^= ch;
        hash *= kPrime;
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

} // namespace

Storage::Storage(userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

// String helpers
std::string Storage::Trim(const std::string& value) {
    const auto begin =
        std::find_if_not(value.begin(), value.end(),
                         [](unsigned char ch) { return std::isspace(ch) != 0; });
    const auto end =
        std::find_if_not(value.rbegin(), value.rend(),
                         [](unsigned char ch) { return std::isspace(ch) != 0; })
            .base();
    if (begin >= end) return {};
    return std::string(begin, end);
}

std::string Storage::Normalize(const std::string& value) {
    return ToLower(Trim(value));
}

void Storage::ValidateDate(const std::string& value,
                           const std::string& field_name) {
    static const std::regex kDatePattern(R"(^\d{4}-\d{2}-\d{2}$)");
    if (!std::regex_match(value, kDatePattern)) {
        throw std::runtime_error("Field '" + field_name +
                                 "' must be in YYYY-MM-DD format");
    }
}

std::string Storage::HashPassword(const std::string& value) {
    return Fnv1aHex("password:" + value);
}

std::string Storage::GenerateToken(std::string_view login,
                                   std::int64_t user_id) {
    const auto seed = std::string(login) + ":" + std::to_string(user_id) +
                      ":" + Fnv1aHex(login);
    return "session-" + Fnv1aHex(seed);
}

// Row parsers
User Storage::ParseUserRow(const userver::storages::postgres::Row& row) const {
    User user;
    user.id            = row["id"].As<std::int64_t>();
    user.login         = row["login"].As<std::string>();
    user.name          = row["name"].As<std::string>();
    user.surname       = row["surname"].As<std::string>();
    user.email         = row["email"].As<std::string>();
    user.password_hash = row["password_hash"].As<std::string>();
    return user;
}

Hotel Storage::ParseHotelRow(const userver::storages::postgres::Row& row) const {
    Hotel hotel;
    hotel.id                 = row["id"].As<std::int64_t>();
    hotel.name               = row["name"].As<std::string>();
    hotel.city               = row["city"].As<std::string>();
    hotel.address            = row["address"].As<std::string>();
    hotel.description        = row["description"].As<std::string>();
    hotel.created_by_user_id = row["created_by_user_id"].As<std::int64_t>();
    return hotel;
}

Booking Storage::ParseBookingRow(const userver::storages::postgres::Row& row) const {
    Booking booking;
    booking.id        = row["id"].As<std::int64_t>();
    booking.user_id   = row["user_id"].As<std::int64_t>();
    booking.hotel_id  = row["hotel_id"].As<std::int64_t>();
    booking.check_in  = row["check_in"].As<std::string>();
    booking.check_out = row["check_out"].As<std::string>();

    const auto status = row["status"].As<std::string>();
    booking.status = (status == "cancelled") ? BookingStatus::kCancelled
                                             : BookingStatus::kActive;
    return booking;
}

// ---- User operations ----
User Storage::RegisterUser(const RegisterUserRequest& request) {
    const auto login    = Normalize(request.login);
    const auto name     = Trim(request.name);
    const auto surname  = Trim(request.surname);
    const auto email    = Trim(request.email);
    const auto password = request.password;

    if (login.empty() || name.empty() || surname.empty() ||
        email.empty() || password.empty()) {
        throw std::runtime_error("login, name, surname, email and password are required");
    }

    const auto existing = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id FROM users WHERE login=$1", login);
    if (!existing.IsEmpty()) {
        throw std::runtime_error("User with this login already exists");
    }

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO users(login, name, surname, email, password_hash) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, login, name, surname, email, password_hash",
        login, name, surname, email, HashPassword(password));

    return ParseUserRow(result.Front());
}

Session Storage::Login(const LoginRequest& request) {
    const auto login = Normalize(request.login);
    if (login.empty() || request.password.empty()) {
        throw std::runtime_error("login and password are required");
    }

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, login, password_hash FROM users WHERE login=$1", login);
    if (result.IsEmpty()) {
        throw std::runtime_error("Invalid login or password");
    }

    const auto row            = result.Front();
    const auto password_hash  = row["password_hash"].As<std::string>();

    if (password_hash != HashPassword(request.password)) {
        throw std::runtime_error("Invalid login or password");
    }

    Session session;
    session.user_id = row["id"].As<std::int64_t>();
    session.login   = row["login"].As<std::string>();
    session.token   = GenerateToken(session.login, session.user_id);

    pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO sessions(token, user_id, login) VALUES($1, $2, $3) "
        "ON CONFLICT (token) DO UPDATE SET user_id = EXCLUDED.user_id, "
        "login = EXCLUDED.login",
        session.token, session.user_id, session.login);

    return session;
}

std::optional<Session> Storage::Authenticate(const std::string& token) const {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT token, user_id, login FROM sessions WHERE token=$1", token);
    if (result.IsEmpty()) return std::nullopt;

    Session session;
    const auto row  = result.Front();
    session.token   = row["token"].As<std::string>();
    session.user_id = row["user_id"].As<std::int64_t>();
    session.login   = row["login"].As<std::string>();
    return session;
}

std::optional<User> Storage::GetUserById(std::int64_t user_id) const {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, login, name, surname, email, password_hash "
        "FROM users WHERE id=$1",
        user_id);
    if (result.IsEmpty()) return std::nullopt;
    return ParseUserRow(result.Front());
}

std::optional<User> Storage::FindUserByLogin(const std::string& login) const {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, login, name, surname, email, password_hash "
        "FROM users WHERE login=$1",
        Normalize(login));
    if (result.IsEmpty()) return std::nullopt;
    return ParseUserRow(result.Front());
}

std::vector<User> Storage::SearchUsers(const std::string& name_mask,
                                       const std::string& surname_mask) const {
    // Статический запрос с двумя параметрами, пустые маски пропускаются через ILIKE
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, login, name, surname, email, password_hash "
        "FROM users "
        "WHERE (TRIM($1) = '' OR name ILIKE '%' || $1 || '%') "
        "  AND (TRIM($2) = '' OR surname ILIKE '%' || $2 || '%') "
        "ORDER BY surname, name",
        name_mask, surname_mask);

    std::vector<User> users;
    for (const auto& row : result) {
        users.push_back(ParseUserRow(row));
    }
    return users;
}

// ---- Hotel operations ----
Hotel Storage::CreateHotel(const CreateHotelRequest& request,
                           std::int64_t created_by_user_id) {
    const auto name        = Trim(request.name);
    const auto city        = Trim(request.city);
    const auto address     = Trim(request.address);
    const auto description = Trim(request.description);

    if (name.empty() || city.empty() || address.empty()) {
        throw std::runtime_error("name, city and address are required");
    }

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO hotels(name, city, address, description, created_by_user_id) "
        "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id, name, city, address, description, created_by_user_id",
        name, city, address, description, created_by_user_id);

    return ParseHotelRow(result.Front());
}

std::optional<Hotel> Storage::GetHotelById(std::int64_t hotel_id) const {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, name, city, address, description, created_by_user_id "
        "FROM hotels WHERE id=$1",
        hotel_id);
    if (result.IsEmpty()) return std::nullopt;
    return ParseHotelRow(result.Front());
}

std::vector<Hotel> Storage::ListHotels(const std::optional<std::string>& city) const {
    // Используем optional, передаётся в запрос как есть (NULL если нет значения)
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, name, city, address, description, created_by_user_id "
        "FROM hotels "
        "WHERE ($1 IS NULL OR LOWER(city) = LOWER($1)) "
        "ORDER BY rating DESC NULLS LAST, name",
        city);

    std::vector<Hotel> hotels;
    for (const auto& row : result) {
        hotels.push_back(ParseHotelRow(row));
    }
    return hotels;
}

// ---- Booking operations ----
Booking Storage::CreateBooking(const CreateBookingRequest& request,
                               std::int64_t user_id) {
    ValidateDate(request.check_in, "check_in");
    ValidateDate(request.check_out, "check_out");

    if (request.check_in >= request.check_out) {
        throw std::runtime_error("check_in must be before check_out");
    }

    const auto hotel = GetHotelById(request.hotel_id);
    if (!hotel) {
        throw std::runtime_error("Hotel not found");
    }

    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO bookings(user_id, hotel_id, check_in, check_out, status) "
        "VALUES ($1, $2, $3, $4, 'active') "
        "RETURNING id, user_id, hotel_id, check_in, check_out, status",
        user_id, request.hotel_id, request.check_in, request.check_out);

    return ParseBookingRow(result.Front());
}

std::vector<Booking> Storage::ListUserBookings(std::int64_t user_id) const {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, user_id, hotel_id, check_in, check_out, status "
        "FROM bookings WHERE user_id=$1",
        user_id);

    std::vector<Booking> bookings;
    for (const auto& row : result) {
        bookings.push_back(ParseBookingRow(row));
    }
    return bookings;
}

void Storage::CancelBooking(std::int64_t requester_user_id,
                            std::int64_t booking_id) {
    const auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, user_id, hotel_id, check_in, check_out, status "
        "FROM bookings WHERE id=$1",
        booking_id);

    if (result.IsEmpty()) {
        throw std::runtime_error("Booking not found");
    }

    const auto booking = ParseBookingRow(result.Front());
    if (booking.user_id != requester_user_id) {
        throw std::runtime_error("You can cancel only your own bookings");
    }

    if (booking.status == BookingStatus::kCancelled) {
        throw std::runtime_error("Booking is already cancelled");
    }

    pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "UPDATE bookings SET status='cancelled' WHERE id=$1", booking_id);
}
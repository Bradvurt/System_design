#include "models.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

// --------------- Storage Singleton ---------------

Storage& Storage::Instance() {
    static Storage instance;
    return instance;
}

// --------------- String Helpers ---------------

std::string Storage::ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string Storage::Trim(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) return {};
    return std::string(begin, end);
}

std::string Storage::Normalize(const std::string& value) {
    return ToLower(Trim(value));
}

bool Storage::IsBlank(const std::string& value) {
    return Trim(value).empty();
}

bool Storage::ContainsCaseInsensitive(std::string_view haystack,
                                       std::string_view needle) {
    return ToLower(std::string(haystack)).find(ToLower(std::string(needle))) !=
           std::string::npos;
}

bool Storage::MaskMatches(const std::string& value, const std::string& mask) {
    if (mask.empty()) return true;
    return ToLower(value).find(ToLower(mask)) != std::string::npos;
}

// --------------- User Operations ---------------

std::optional<User> Storage::AddUser(std::string login, std::string name,
                                     std::string surname, std::string email,
                                     std::string password_hash) {
    std::lock_guard lock(mutex_);

    // Check for duplicate login
    for (const auto& u : users_) {
        if (Normalize(u.login) == Normalize(login)) {
            return std::nullopt;
        }
    }

    User user;
    user.id = next_user_id_++;
    user.login = login;
    user.name = name;
    user.surname = surname;
    user.email = email;
    user.password_hash = password_hash;
    users_.push_back(user);
    return user;
}

std::optional<User> Storage::FindUserByLogin(const std::string& login) const {
    std::lock_guard lock(mutex_);
    auto normalized = Normalize(login);
    for (const auto& u : users_) {
        if (Normalize(u.login) == normalized) {
            return u;
        }
    }
    return std::nullopt;
}

std::optional<User> Storage::FindUserById(std::int64_t user_id) const {
    std::lock_guard lock(mutex_);
    for (const auto& u : users_) {
        if (u.id == user_id) return u;
    }
    return std::nullopt;
}

std::vector<User> Storage::SearchUsers(const std::string& name_mask,
                                        const std::string& surname_mask) const {
    std::lock_guard lock(mutex_);
    std::vector<User> result;
    for (const auto& u : users_) {
        if (!MaskMatches(u.name, name_mask)) continue;
        if (!MaskMatches(u.surname, surname_mask)) continue;
        result.push_back(u);
    }
    return result;
}

bool Storage::UserExists(std::int64_t user_id) const {
    return FindUserById(user_id).has_value();
}

// --------------- Hotel Operations ---------------

std::optional<Hotel> Storage::AddHotel(std::string name, std::string city,
                                        std::string address,
                                        std::string description,
                                        std::int64_t created_by_user_id) {
    std::lock_guard lock(mutex_);
    Hotel hotel;
    hotel.id = next_hotel_id_++;
    hotel.name = name;
    hotel.city = city;
    hotel.address = address;
    hotel.description = description;
    hotel.created_by_user_id = created_by_user_id;
    hotels_.push_back(hotel);
    return hotel;
}

std::vector<Hotel> Storage::ListHotels(const std::string& city_filter) const {
    std::lock_guard lock(mutex_);
    std::vector<Hotel> result;
    for (const auto& h : hotels_) {
        if (!city_filter.empty() && Normalize(h.city) != Normalize(city_filter))
            continue;
        result.push_back(h);
    }
    return result;
}

std::optional<Hotel> Storage::FindHotelById(std::int64_t hotel_id) const {
    std::lock_guard lock(mutex_);
    for (const auto& h : hotels_) {
        if (h.id == hotel_id) return h;
    }
    return std::nullopt;
}

// --------------- Booking Operations ---------------

std::optional<Booking> Storage::AddBooking(std::int64_t user_id,
                                            std::int64_t hotel_id,
                                            std::string check_in,
                                            std::string check_out) {
    std::lock_guard lock(mutex_);

    // Validate hotel and user exist
    bool hotel_found = false;
    for (const auto& h : hotels_) {
        if (h.id == hotel_id) { hotel_found = true; break; }
    }
    if (!hotel_found) return std::nullopt;

    bool user_found = false;
    for (const auto& u : users_) {
        if (u.id == user_id) { user_found = true; break; }
    }
    if (!user_found) return std::nullopt;

    // Check for date overlaps with active bookings for the same hotel
    for (const auto& b : bookings_) {
        if (b.hotel_id != hotel_id || !b.active) continue;
        // Overlap: !(check_out <= b.check_in || check_in >= b.check_out)
        bool overlap = !(check_out <= b.check_in || check_in >= b.check_out);
        if (overlap) return std::nullopt;
    }

    Booking booking;
    booking.id = next_booking_id_++;
    booking.user_id = user_id;
    booking.hotel_id = hotel_id;
    booking.check_in = check_in;
    booking.check_out = check_out;
    booking.active = true;
    bookings_.push_back(booking);
    return booking;
}

std::vector<Booking> Storage::ListUserBookings(std::int64_t user_id) const {
    std::lock_guard lock(mutex_);
    std::vector<Booking> result;
    for (const auto& b : bookings_) {
        if (b.user_id == user_id) result.push_back(b);
    }
    return result;
}

std::optional<Booking> Storage::FindBookingById(std::int64_t booking_id) const {
    std::lock_guard lock(mutex_);
    for (const auto& b : bookings_) {
        if (b.id == booking_id) return b;
    }
    return std::nullopt;
}

bool Storage::CancelBooking(std::int64_t booking_id) {
    std::lock_guard lock(mutex_);
    for (auto& b : bookings_) {
        if (b.id == booking_id && b.active) {
            b.active = false;
            return true;
        }
    }
    return false;
}

// --------------- Session Operations ---------------

void Storage::StoreSession(const Session& session) {
    std::lock_guard lock(mutex_);
    // Remove existing session for the same user
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
                       [&](const Session& s) { return s.user_id == session.user_id; }),
        sessions_.end());
    sessions_.push_back(session);
}

std::optional<Session> Storage::FindSession(const std::string& token) const {
    std::lock_guard lock(mutex_);
    for (const auto& s : sessions_) {
        if (s.token == token) return s;
    }
    return std::nullopt;
}
#include "domain/hotel_booking_service.hpp"

#include <userver/formats/bson.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/logging/log.hpp>

#include <fmt/format.h>
#include <stdexcept>
#include <openssl/evp.h>
#include <random>
#include <iomanip>
#include <sstream>

namespace booking {

namespace {

std::string HashPassword(std::string_view password) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_MD_CTX");
    if (1 != EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) ||
        1 != EVP_DigestUpdate(ctx, password.data(), password.size()) ||
        1 != EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("SHA256 hashing failed");
    }
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string GenerateToken(std::string_view login, std::int64_t user_id) {
    // Simple token: base64(login:user_id:random) but for brevity use a hex string
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dist;
    std::stringstream ss;
    ss << std::hex << user_id << ":" << login << ":" << dist(gen);
    return HashPassword(ss.str()); // reuse hash as token
}

std::string NowISO() {
    return userver::utils::datetime::Now().Underlying().Format("%Y-%m-%dT%H:%M:%SZ");
}

}  // namespace

HotelBookingService::HotelBookingService(
    userver::storages::mongo::PoolPtr mongo_pool,
    std::shared_ptr<userver::urabbitmq::Client> rmq_client)
    : mongo_pool_(std::move(mongo_pool)),
      rmq_client_(std::move(rmq_client)) {}

// ---------- Private helpers ----------

std::int64_t HotelBookingService::NextId(const std::string& collection_name) {
    auto counters = mongo_pool_->GetCollection("counters");
    auto filter = userver::formats::bson::MakeDoc("_id", collection_name);
    auto update = userver::formats::bson::MakeDoc("$inc", userver::formats::bson::MakeDoc("seq", 1));
    auto opts = userver::storages::mongo::options::FindOneAndUpdate{}
                    .SetReturnNew(true)
                    .SetUpsert(true);
    auto result = counters.FindOneAndUpdate(filter, update, opts);
    if (!result) {
        throw std::runtime_error(fmt::format("Failed to generate ID for {}", collection_name));
    }
    return result->GetInt64("seq");
}

void HotelBookingService::PublishEvent(const std::string& routing_key,
                                       userver::formats::json::ValueBuilder payload) const {
    if (!rmq_client_) return;  // Event publishing is optional
    payload["event_type"] = routing_key;
    std::string message = userver::formats::json::ToString(payload.ExtractValue());
    try {
        rmq_client_->Publish(
            userver::urabbitmq::Exchange{"booking_events"},
            routing_key,
            message,
            userver::engine::Deadline::FromDuration(std::chrono::seconds{2}));
        LOG_INFO() << "Event published: " << routing_key;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to publish event " << routing_key << ": " << e.what();
    }
}

// ---------- Command methods ----------

User HotelBookingService::RegisterUser(const RegisterUserRequest& request) {
    auto users = mongo_pool_->GetCollection("users");

    // Check uniqueness
    auto existing = users.FindOne(
        userver::formats::bson::MakeDoc("login", request.login));
    if (existing) {
        throw std::runtime_error("User with this login already exists");
    }

    existing = users.FindOne(
        userver::formats::bson::MakeDoc("email", request.email));
    if (existing) {
        throw std::runtime_error("User with this email already exists");
    }

    std::string now = NowISO();
    auto id = NextId("users");
    auto password_hash = HashPassword(request.password);

    userver::formats::bson::ValueBuilder user_doc;
    user_doc["_id"] = id;
    user_doc["login"] = request.login;
    user_doc["first_name"] = request.first_name;
    user_doc["last_name"] = request.last_name;
    user_doc["email"] = request.email;
    user_doc["password_hash"] = password_hash;
    user_doc["created_at"] = now;
    user_doc["updated_at"] = now;

    users.InsertOne(user_doc.ExtractValue());

    User user;
    user.id = id;
    user.login = request.login;
    user.first_name = request.first_name;
    user.last_name = request.last_name;
    user.email = request.email;
    user.phone = "";
    user.password_hash = password_hash;
    user.created_at = now;
    user.updated_at = now;

    // Publish event
    userver::formats::json::ValueBuilder payload;
    payload["user_id"] = id;
    payload["login"] = request.login;
    payload["email"] = request.email;
    PublishEvent("user.registered", std::move(payload));

    return user;
}

Session HotelBookingService::Login(const LoginRequest& request) {
    auto users = mongo_pool_->GetCollection("users");
    auto user_doc = users.FindOne(
        userver::formats::bson::MakeDoc("login", request.login));
    if (!user_doc) {
        throw std::runtime_error("Invalid login or password");
    }

    auto password_hash = user_doc->GetString("password_hash");
    if (HashPassword(request.password) != password_hash) {
        throw std::runtime_error("Invalid login or password");
    }

    auto user_id = user_doc->GetInt64("_id");
    auto token = GenerateToken(request.login, user_id);

    Session session;
    session.token = token;
    session.user_id = user_id;
    session.login = request.login;

    {
        std::lock_guard lock(mutex_);
        sessions_[token] = session;
    }

    return session;
}

std::optional<Session> HotelBookingService::Authenticate(const std::string& token) const {
    std::lock_guard lock(mutex_);
    auto it = sessions_.find(token);
    if (it != sessions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

Hotel HotelBookingService::CreateHotel(const CreateHotelRequest& request,
                                       std::int64_t created_by_user_id) {
    auto hotels = mongo_pool_->GetCollection("hotels");
    std::string now = NowISO();
    auto id = NextId("hotels");

    userver::formats::bson::ValueBuilder hotel_doc;
    hotel_doc["_id"] = id;
    hotel_doc["name"] = request.name;
    hotel_doc["city"] = request.city;
    hotel_doc["address"] = request.address;
    hotel_doc["description"] = request.description;
    hotel_doc["created_by_user_id"] = created_by_user_id;
    hotel_doc["created_at"] = now;
    hotel_doc["updated_at"] = now;

    userver::formats::bson::ValueBuilder rooms_arr(userver::formats::common::Type::kArray);
    for (const auto& room : request.rooms) {
        userver::formats::bson::ValueBuilder room_doc;
        room_doc["room_number"] = room.room_number;
        room_doc["type"] = room.type;
        room_doc["price_per_night"] = room.price_per_night;
        room_doc["capacity"] = room.capacity;
        room_doc["is_available"] = room.is_available;
        rooms_arr.PushBack(room_doc.ExtractValue());
    }
    hotel_doc["rooms"] = rooms_arr.ExtractValue();

    hotels.InsertOne(hotel_doc.ExtractValue());

    Hotel hotel;
    hotel.id = id;
    hotel.name = request.name;
    hotel.city = request.city;
    hotel.address = request.address;
    hotel.description = request.description;
    hotel.created_by_user_id = created_by_user_id;
    hotel.created_at = now;
    hotel.updated_at = now;
    hotel.rooms = request.rooms;

    // Publish event
    userver::formats::json::ValueBuilder payload;
    payload["hotel_id"] = id;
    payload["name"] = request.name;
    payload["city"] = request.city;
    payload["stars"] = 0; // no stars in request, could be extended
    PublishEvent("hotel.created", std::move(payload));

    return hotel;
}

Booking HotelBookingService::CreateBooking(const CreateBookingRequest& request,
                                           std::int64_t user_id) {
    auto hotels = mongo_pool_->GetCollection("hotels");
    auto bookings = mongo_pool_->GetCollection("bookings");

    // Verify hotel exists
    auto hotel_doc = hotels.FindOne(
        userver::formats::bson::MakeDoc("_id", request.hotel_id));
    if (!hotel_doc) {
        throw std::runtime_error("Hotel not found");
    }

    // Find the room and check availability
    bool room_found = false;
    double room_price = 0.0;
    auto rooms = hotel_doc->GetSubarray("rooms");
    for (const auto& room_bson : rooms) {
        if (room_bson.GetString("room_number") == request.room_number) {
            if (!room_bson.GetBool("is_available", false)) {
                throw std::runtime_error("Room is not available");
            }
            room_found = true;
            room_price = room_bson.GetDouble("price_per_night");
            break;
        }
    }
    if (!room_found) {
        throw std::runtime_error("Room not found in hotel");
    }

    // Check for overlapping bookings (simplified)
    auto filter = userver::formats::bson::MakeDoc(
        "hotel_id", request.hotel_id,
        "room_number", request.room_number,
        "status", "active",
        "$or", userver::formats::bson::MakeArray(
            userver::formats::bson::MakeDoc(
                "check_in", userver::formats::bson::MakeDoc("$lt", request.check_out),
                "check_out", userver::formats::bson::MakeDoc("$gt", request.check_in)
            )
        )
    );
    auto conflict = bookings.FindOne(filter);
    if (conflict) {
        throw std::runtime_error("Room is already booked for these dates");
    }

    std::string now = NowISO();
    auto id = NextId("bookings");

    // Calculate total price (simplified, assuming inclusive nights)
    // In a real system parse dates and compute difference
    int nights = 1; // placeholder
    double total_price = nights * room_price;

    userver::formats::bson::ValueBuilder booking_doc;
    booking_doc["_id"] = id;
    booking_doc["user_id"] = user_id;
    booking_doc["hotel_id"] = request.hotel_id;
    booking_doc["room_number"] = request.room_number;
    booking_doc["check_in"] = request.check_in;
    booking_doc["check_out"] = request.check_out;
    booking_doc["status"] = "active";
    booking_doc["total_price"] = total_price;
    booking_doc["created_at"] = now;
    booking_doc["updated_at"] = now;

    bookings.InsertOne(booking_doc.ExtractValue());

    Booking booking;
    booking.id = id;
    booking.user_id = user_id;
    booking.hotel_id = request.hotel_id;
    booking.room_number = request.room_number;
    booking.check_in = request.check_in;
    booking.check_out = request.check_out;
    booking.status = BookingStatus::kActive;
    booking.total_price = total_price;
    booking.created_at = now;
    booking.updated_at = now;

    // Publish event
    userver::formats::json::ValueBuilder payload;
    payload["booking_id"] = std::to_string(id);
    payload["user_id"] = user_id;
    payload["hotel_id"] = request.hotel_id;
    payload["check_in"] = request.check_in;
    payload["check_out"] = request.check_out;
    payload["status"] = "active";
    PublishEvent("booking.created", std::move(payload));

    return booking;
}

void HotelBookingService::CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id) {
    auto bookings = mongo_pool_->GetCollection("bookings");
    auto filter = userver::formats::bson::MakeDoc("_id", booking_id);
    auto booking_doc = bookings.FindOne(filter);
    if (!booking_doc) {
        throw std::runtime_error("Booking not found");
    }
    if (booking_doc->GetInt64("user_id") != requester_user_id) {
        throw std::runtime_error("You can only cancel your own bookings");
    }
    if (booking_doc->GetString("status") != "active") {
        throw std::runtime_error("Only active bookings can be cancelled");
    }

    auto update = userver::formats::bson::MakeDoc(
        "$set", userver::formats::bson::MakeDoc(
            "status", "cancelled",
            "updated_at", NowISO()
        )
    );
    bookings.UpdateOne(filter, update);

    // Publish event
    userver::formats::json::ValueBuilder payload;
    payload["booking_id"] = std::to_string(booking_id);
    payload["user_id"] = requester_user_id;
    PublishEvent("booking.cancelled", std::move(payload));
}

// ---------- Query methods ----------

std::optional<User> HotelBookingService::GetUserById(std::int64_t user_id) const {
    auto users = mongo_pool_->GetCollection("users");
    auto doc = users.FindOne(userver::formats::bson::MakeDoc("_id", user_id));
    if (!doc) return std::nullopt;

    User user;
    user.id = doc->GetInt64("_id");
    user.login = doc->GetString("login");
    user.first_name = doc->GetString("first_name");
    user.last_name = doc->GetString("last_name");
    user.email = doc->GetString("email");
    user.phone = doc->GetString("phone", "");
    user.password_hash = doc->GetString("password_hash");
    user.created_at = doc->GetString("created_at");
    user.updated_at = doc->GetString("updated_at");
    return user;
}

std::optional<User> HotelBookingService::FindUserByLogin(const std::string& login) const {
    auto users = mongo_pool_->GetCollection("users");
    auto doc = users.FindOne(userver::formats::bson::MakeDoc("login", login));
    if (!doc) return std::nullopt;

    User user;
    user.id = doc->GetInt64("_id");
    user.login = doc->GetString("login");
    user.first_name = doc->GetString("first_name");
    user.last_name = doc->GetString("last_name");
    user.email = doc->GetString("email");
    user.phone = doc->GetString("phone", "");
    user.password_hash = doc->GetString("password_hash");
    user.created_at = doc->GetString("created_at");
    user.updated_at = doc->GetString("updated_at");
    return user;
}

std::vector<User> HotelBookingService::SearchUsers(const std::string& name_mask,
                                                   const std::string& surname_mask) const {
    auto users = mongo_pool_->GetCollection("users");
    userver::formats::bson::ValueBuilder filter;

    // Build regex conditions for name and surname masks
    if (!name_mask.empty() && !surname_mask.empty()) {
        filter["$and"] = userver::formats::bson::MakeArray(
            userver::formats::bson::MakeDoc("first_name", userver::formats::bson::MakeDoc("$regex", name_mask, "$options", "i")),
            userver::formats::bson::MakeDoc("last_name", userver::formats::bson::MakeDoc("$regex", surname_mask, "$options", "i"))
        );
    } else if (!name_mask.empty()) {
        filter["first_name"] = userver::formats::bson::MakeDoc("$regex", name_mask, "$options", "i");
    } else if (!surname_mask.empty()) {
        filter["last_name"] = userver::formats::bson::MakeDoc("$regex", surname_mask, "$options", "i");
    } else {
        // Both empty -> match all (empty filter)
    }

    std::vector<User> result;
    auto cursor = users.Find(filter.ExtractValue());
    for (const auto& doc : cursor) {
        User user;
        user.id = doc.GetInt64("_id");
        user.login = doc.GetString("login");
        user.first_name = doc.GetString("first_name");
        user.last_name = doc.GetString("last_name");
        user.email = doc.GetString("email");
        user.phone = doc.GetString("phone", "");
        user.password_hash = doc.GetString("password_hash");
        user.created_at = doc.GetString("created_at");
        user.updated_at = doc.GetString("updated_at");
        result.push_back(std::move(user));
    }
    return result;
}

std::optional<Hotel> HotelBookingService::GetHotelById(std::int64_t hotel_id) const {
    auto hotels = mongo_pool_->GetCollection("hotels");
    auto doc = hotels.FindOne(userver::formats::bson::MakeDoc("_id", hotel_id));
    if (!doc) return std::nullopt;

    Hotel hotel;
    hotel.id = doc->GetInt64("_id");
    hotel.name = doc->GetString("name");
    hotel.city = doc->GetString("city");
    hotel.address = doc->GetString("address");
    hotel.description = doc->GetString("description", "");
    hotel.stars = static_cast<int>(doc->GetInt64("stars", 0));
    hotel.created_by_user_id = doc->GetInt64("created_by_user_id");
    hotel.created_at = doc->GetString("created_at");
    hotel.updated_at = doc->GetString("updated_at");

    if (doc->HasField("rooms")) {
        auto rooms_arr = doc->GetSubarray("rooms");
        for (const auto& room_bson : rooms_arr) {
            Room room;
            room.room_number = room_bson.GetString("room_number");
            room.type = room_bson.GetString("type");
            room.price_per_night = room_bson.GetDouble("price_per_night");
            room.capacity = static_cast<int>(room_bson.GetInt64("capacity"));
            room.is_available = room_bson.GetBool("is_available", true);
            hotel.rooms.push_back(std::move(room));
        }
    }
    return hotel;
}

std::vector<Hotel> HotelBookingService::ListHotels(const std::optional<std::string>& city) const {
    auto hotels = mongo_pool_->GetCollection("hotels");
    userver::formats::bson::ValueBuilder filter;
    if (city.has_value() && !city->empty()) {
        filter["city"] = *city;
    }

    std::vector<Hotel> result;
    auto cursor = hotels.Find(filter.ExtractValue());
    for (const auto& doc : cursor) {
        Hotel hotel;
        hotel.id = doc.GetInt64("_id");
        hotel.name = doc.GetString("name");
        hotel.city = doc.GetString("city");
        hotel.address = doc.GetString("address");
        hotel.description = doc.GetString("description", "");
        hotel.stars = static_cast<int>(doc.GetInt64("stars", 0));
        hotel.created_by_user_id = doc.GetInt64("created_by_user_id");
        hotel.created_at = doc.GetString("created_at");
        hotel.updated_at = doc.GetString("updated_at");

        if (doc.HasField("rooms")) {
            auto rooms_arr = doc.GetSubarray("rooms");
            for (const auto& room_bson : rooms_arr) {
                Room room;
                room.room_number = room_bson.GetString("room_number");
                room.type = room_bson.GetString("type");
                room.price_per_night = room_bson.GetDouble("price_per_night");
                room.capacity = static_cast<int>(room_bson.GetInt64("capacity"));
                room.is_available = room_bson.GetBool("is_available", true);
                hotel.rooms.push_back(std::move(room));
            }
        }
        result.push_back(std::move(hotel));
    }
    return result;
}

std::vector<Booking> HotelBookingService::ListUserBookings(std::int64_t requester_user_id,
                                                           std::int64_t target_user_id) const {
    // Permission check: users can only see their own bookings (simple policy)
    if (requester_user_id != target_user_id) {
        throw std::runtime_error("You are not allowed to view other user's bookings");
    }

    auto bookings = mongo_pool_->GetCollection("bookings");
    auto filter = userver::formats::bson::MakeDoc("user_id", target_user_id);
    std::vector<Booking> result;
    auto cursor = bookings.Find(filter);
    for (const auto& doc : cursor) {
        Booking booking;
        booking.id = doc.GetInt64("_id");
        booking.user_id = doc.GetInt64("user_id");
        booking.hotel_id = doc.GetInt64("hotel_id");
        booking.room_number = doc.GetString("room_number");
        booking.check_in = doc.GetString("check_in");
        booking.check_out = doc.GetString("check_out");
        std::string status_str = doc.GetString("status");
        if (status_str == "active") booking.status = BookingStatus::kActive;
        else if (status_str == "cancelled") booking.status = BookingStatus::kCancelled;
        else if (status_str == "completed") booking.status = BookingStatus::kCompleted;
        booking.total_price = doc.GetDouble("total_price");
        booking.created_at = doc.GetString("created_at");
        booking.updated_at = doc.GetString("updated_at");
        result.push_back(std::move(booking));
    }
    return result;
}

}  // namespace booking
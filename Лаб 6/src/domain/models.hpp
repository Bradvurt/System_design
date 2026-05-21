#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace booking {

enum class BookingStatus {
    kActive,
    kCancelled,
    kCompleted
};

inline std::string ToString(BookingStatus status) {
    switch (status) {
        case BookingStatus::kActive: return "active";
        case BookingStatus::kCancelled: return "cancelled";
        case BookingStatus::kCompleted: return "completed";
    }
    return "unknown";
}

struct User {
    std::int64_t id{0};
    std::string login;
    std::string first_name;
    std::string last_name;
    std::string email;
    std::string phone;
    std::string password_hash;
    std::string created_at;
    std::string updated_at;
};

struct Room {
    std::string room_number;
    std::string type;
    double price_per_night{0.0};
    int capacity{0};
    bool is_available{true};
};

struct Hotel {
    std::int64_t id{0};
    std::string name;
    std::string description;
    std::string city;
    std::string address;
    int stars{0};
    std::int64_t created_by_user_id{0};
    std::string created_at;
    std::string updated_at;
    std::vector<Room> rooms;
};

struct Booking {
    std::int64_t id{0};
    std::int64_t user_id{0};
    std::int64_t hotel_id{0};
    std::string room_number;
    std::string check_in;
    std::string check_out;
    BookingStatus status{BookingStatus::kActive};
    double total_price{0.0};
    std::string created_at;
    std::string updated_at;
};

struct Session {
    std::string token;
    std::int64_t user_id{0};
    std::string login;
};

// Request DTOs
struct RegisterUserRequest {
    std::string login;
    std::string password;
    std::string first_name;
    std::string last_name;
    std::string email;
};

struct LoginRequest {
    std::string login;
    std::string password;
};

struct CreateHotelRequest {
    std::string name;
    std::string city;
    std::string address;
    std::string description;
    std::vector<Room> rooms;
};

struct CreateBookingRequest {
    std::int64_t hotel_id{0};
    std::string room_number;
    std::string check_in;
    std::string check_out;
};

} // namespace booking
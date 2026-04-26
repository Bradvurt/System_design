#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace lab4 {

enum class ServiceErrorKind {
  kBadRequest,
  kUnauthorized,
  kForbidden,
  kNotFound,
  kConflict,
};

class ServiceError final : public std::runtime_error {
 public:
  ServiceError(ServiceErrorKind kind, std::string code, std::string message)
      : std::runtime_error(std::move(message)),
        kind_(kind),
        code_(std::move(code)) {}

  ServiceErrorKind GetKind() const noexcept { return kind_; }
  const std::string& GetCode() const noexcept { return code_; }

 private:
  ServiceErrorKind kind_;
  std::string code_;
};

struct User {
  std::int64_t id{0};
  std::string login;
  std::string first_name;   // используется в api_utils.cpp
  std::string last_name;    // используется в api_utils.cpp
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
  std::string checkout_date;
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

enum class BookingStatus {
  kActive,
  kCancelled,
  kCompleted,
};

inline std::string ToString(BookingStatus status) {
  switch (status) {
    case BookingStatus::kActive: return "active";
    case BookingStatus::kCancelled: return "cancelled";
    case BookingStatus::kCompleted: return "completed";
    default: return "unknown";
  }
}

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

struct AuthContext {
  std::int64_t user_id{0};
  std::string login;
};

struct RegisterUserRequest {
  std::string login;
  std::string name;         // маппится на first_name в User
  std::string surname;      // маппится на last_name в User
  std::string email;
  std::string password;
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

}
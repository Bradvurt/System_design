#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/storages/postgres/cluster.hpp>

// ===== Data structures (identical to original lab3) =====
struct User {
    std::int64_t id{0};
    std::string login;
    std::string name;
    std::string surname;
    std::string email;
    std::string password_hash;
};

struct Hotel {
    std::int64_t id{0};
    std::string name;
    std::string city;
    std::string address;
    std::string description;
    std::int64_t created_by_user_id{0};
};

enum class BookingStatus { kActive, kCancelled };

inline std::string ToString(BookingStatus status) {
    return status == BookingStatus::kActive ? "active" : "cancelled";
}

struct Booking {
    std::int64_t id{0};
    std::int64_t user_id{0};
    std::int64_t hotel_id{0};
    std::string check_in;
    std::string check_out;
    BookingStatus status{BookingStatus::kActive};
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

// ===== Request objects =====
struct RegisterUserRequest {
    std::string login;
    std::string name;
    std::string surname;
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
};

struct CreateBookingRequest {
    std::int64_t hotel_id{0};
    std::string check_in;
    std::string check_out;
};

// ===== Storage class – wraps all PostgreSQL operations =====
class Storage {
public:
    explicit Storage(userver::storages::postgres::ClusterPtr pg_cluster);

    // User operations
    User RegisterUser(const RegisterUserRequest& request);
    Session Login(const LoginRequest& request);
    std::optional<Session> Authenticate(const std::string& token) const;
    std::optional<User> GetUserById(std::int64_t user_id) const;
    std::optional<User> FindUserByLogin(const std::string& login) const;
    std::vector<User> SearchUsers(const std::string& name_mask,
                                  const std::string& surname_mask) const;

    // Hotel operations
    Hotel CreateHotel(const CreateHotelRequest& request,
                      std::int64_t created_by_user_id);
    std::optional<Hotel> GetHotelById(std::int64_t hotel_id) const;
    std::vector<Hotel> ListHotels(const std::optional<std::string>& city) const;

    // Booking operations
    Booking CreateBooking(const CreateBookingRequest& request,
                          std::int64_t user_id);
    std::vector<Booking> ListUserBookings(std::int64_t user_id) const;
    void CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id);

private:
    static std::string Normalize(const std::string& value);
    static std::string Trim(const std::string& value);
    static void ValidateDate(const std::string& value,
                             const std::string& field_name);
    static std::string HashPassword(const std::string& value);
    static std::string GenerateToken(std::string_view login,
                                     std::int64_t user_id);

    User ParseUserRow(const userver::storages::postgres::Row& row) const;
    Hotel ParseHotelRow(const userver::storages::postgres::Row& row) const;
    Booking ParseBookingRow(const userver::storages::postgres::Row& row) const;

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
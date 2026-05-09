#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// --------------- Data Structures ---------------

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

struct Booking {
    std::int64_t id{0};
    std::int64_t user_id{0};
    std::int64_t hotel_id{0};
    std::string check_in;
    std::string check_out;
    bool active{true};  // true = active, false = cancelled
};

struct Session {
    std::string token;
    std::int64_t user_id{0};
    std::string login;
};

// --------------- In-Memory Storage ---------------

class Storage {
public:
    static Storage& Instance();

    // Users
    std::optional<User> AddUser(std::string login, std::string name,
                                std::string surname, std::string email,
                                std::string password_hash);
    std::optional<User> FindUserByLogin(const std::string& login) const;
    std::optional<User> FindUserById(std::int64_t user_id) const;
    std::vector<User> SearchUsers(const std::string& name_mask,
                                  const std::string& surname_mask) const;
    bool UserExists(std::int64_t user_id) const;

    // Hotels
    std::optional<Hotel> AddHotel(std::string name, std::string city,
                                  std::string address, std::string description,
                                  std::int64_t created_by_user_id);
    std::vector<Hotel> ListHotels(const std::string& city_filter) const;
    std::optional<Hotel> FindHotelById(std::int64_t hotel_id) const;

    // Bookings
    std::optional<Booking> AddBooking(std::int64_t user_id,
                                      std::int64_t hotel_id,
                                      std::string check_in,
                                      std::string check_out);
    std::vector<Booking> ListUserBookings(std::int64_t user_id) const;
    std::optional<Booking> FindBookingById(std::int64_t booking_id) const;
    bool CancelBooking(std::int64_t booking_id);

    // Sessions
    void StoreSession(const Session& session);
    std::optional<Session> FindSession(const std::string& token) const;

    // Helpers
    static std::string Normalize(const std::string& value);
    static std::string Trim(const std::string& value);
    static bool IsBlank(const std::string& value);
    static bool ContainsCaseInsensitive(std::string_view haystack,
                                        std::string_view needle);

private:
    Storage() = default;

    static bool MaskMatches(const std::string& value, const std::string& mask);
    static std::string ToLower(std::string s);

    mutable std::mutex mutex_;

    std::vector<User> users_;
    std::vector<Hotel> hotels_;
    std::vector<Booking> bookings_;
    std::vector<Session> sessions_;

    std::int64_t next_user_id_{1};
    std::int64_t next_hotel_id_{1};
    std::int64_t next_booking_id_{1};
};
#pragma once

#include <vector>
#include <optional>
#include <string>
#include <mutex>
#include <unordered_map>
#include <chrono>

struct User {
    int id{};
    std::string login;
    std::string password_hash;
    std::string first_name;
    std::string last_name;
};

struct Hotel {
    int id{};
    std::string name;
    std::string city;
    std::string address;
};

struct Booking {
    int id{};
    int user_id{};
    int hotel_id{};
    std::string check_in;
    std::string check_out;
    bool active{true};
};

class Storage {
public:
    static Storage& instance();

    // User operations
    std::optional<User> addUser(const std::string& login,
                                const std::string& password_hash,
                                const std::string& first_name,
                                const std::string& last_name);
    std::optional<User> findUserByLogin(const std::string& login) const;
    std::vector<User> searchUsers(const std::string& login,
                                  const std::string& first_name_mask,
                                  const std::string& last_name_mask) const;

    // Hotel operations
    std::optional<Hotel> addHotel(const std::string& name,
                                  const std::string& city,
                                  const std::string& address);
    std::vector<Hotel> listHotels(const std::string& city) const;
    std::optional<Hotel> findHotelById(int id) const;

    // Booking operations
    std::optional<Booking> addBooking(int user_id, int hotel_id,
                                     const std::string& check_in,
                                     const std::string& check_out);
    std::vector<Booking> listBookingsByUser(int user_id) const;
    std::optional<Booking> findBookingById(int id) const;
    bool cancelBooking(int booking_id, int user_id);

    // Caching (retained from lab5)
    struct CachedHotelsEntry {
        std::vector<Hotel> hotels;
        std::chrono::steady_clock::time_point expire_at;
    };
    struct CachedUserBookingsEntry {
        std::vector<Booking> bookings;
        std::chrono::steady_clock::time_point expire_at;
    };

    // Rate limiting (retained from lab5)
    struct RateLimitDecision {
        bool allowed{true};
        std::uint32_t limit{20};
        std::uint32_t remaining{20};
        std::int64_t reset_unix_seconds{0};
    };
    RateLimitDecision checkBookingRateLimit(int user_id);

    void clearForTests();

private:
    Storage() = default;

    static bool MaskMatches(const std::string& value, const std::string& mask);
    static std::string ToLower(std::string s);

    void invalidateHotelsCache();
    void invalidateUserBookingsCache(int user_id);
    static std::string buildHotelsCacheKey(const std::optional<std::string>& city_filter);

    mutable std::mutex mutex_;
    std::vector<User> users_;
    std::vector<Hotel> hotels_;
    std::vector<Booking> bookings_;
    int next_user_id_{0};
    int next_hotel_id_{0};
    int next_booking_id_{0};

    // Cache state
    mutable std::unordered_map<std::string, CachedHotelsEntry> hotels_cache_;
    mutable std::unordered_map<int, CachedUserBookingsEntry> user_bookings_cache_;

    // Rate limit state (simple fixed-window counter using a map)
    mutable std::unordered_map<int, std::pair<int, std::chrono::steady_clock::time_point>> rate_limit_counters_;
};
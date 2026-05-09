#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/storages/redis/client.hpp>

#include "models.hpp"

// Cache service using Redis for Cache-Aside pattern
class CacheService {
public:
    explicit CacheService(userver::storages::redis::ClientPtr redis_client);

    // Hotels cache
    std::optional<std::vector<Hotel>> GetHotels(
        const std::optional<std::string>& city_filter);
    void SetHotels(const std::optional<std::string>& city_filter,
                   const std::vector<Hotel>& hotels,
                   std::chrono::seconds ttl);
    void InvalidateHotels();

    // User bookings cache
    std::optional<std::vector<Booking>> GetUserBookings(std::int64_t user_id);
    void SetUserBookings(std::int64_t user_id,
                         const std::vector<Booking>& bookings,
                         std::chrono::seconds ttl);
    void InvalidateUserBookings(std::int64_t user_id);

private:
    static std::string BuildHotelsCacheKey(
        const std::optional<std::string>& city_filter);
    static std::string BuildUserBookingsCacheKey(std::int64_t user_id);

    userver::storages::redis::ClientPtr redis_client_;

    static constexpr std::chrono::seconds kHotelsTtl{60};
    static constexpr std::chrono::seconds kUserBookingsTtl{30};
};
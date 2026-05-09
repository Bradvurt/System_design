#pragma once

#include <cstdint>
#include <memory>
#include <chrono>

#include <userver/storages/redis/client.hpp>

struct RateLimitDecision {
    bool allowed{true};
    std::uint32_t limit{0};
    std::uint32_t remaining{0};
    std::int64_t reset_unix_seconds{0};
};

// Fixed Window Counter rate limiter using Redis
class RateLimiter {
public:
    explicit RateLimiter(userver::storages::redis::ClientPtr redis_client);

    // Check if a booking creation request is allowed for the given user.
    // Window: 60 seconds, Limit: 20 requests per window.
    RateLimitDecision CheckBookingCreation(std::int64_t user_id);

private:
    userver::storages::redis::ClientPtr redis_client_;

    static constexpr std::uint32_t kBookingLimit = 20;
    static constexpr std::chrono::seconds kWindowSize{60};

    static std::string BuildKey(std::int64_t user_id, std::int64_t window_start);
    static std::int64_t CurrentWindowStart();
};
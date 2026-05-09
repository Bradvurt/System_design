#include "rate_limiter.hpp"

#include <chrono>

RateLimiter::RateLimiter(userver::storages::redis::ClientPtr redis_client)
    : redis_client_(std::move(redis_client)) {}

std::string RateLimiter::BuildKey(std::int64_t user_id,
                                   std::int64_t window_start) {
    return "ratelimit:booking:" + std::to_string(user_id) + ":" +
           std::to_string(window_start);
}

std::int64_t RateLimiter::CurrentWindowStart() {
    const auto now = std::chrono::system_clock::now();
    const auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
                             now.time_since_epoch())
                             .count();
    return (now_sec / kWindowSize.count()) * kWindowSize.count();
}

RateLimitDecision RateLimiter::CheckBookingCreation(std::int64_t user_id) {
    RateLimitDecision decision;
    decision.limit = kBookingLimit;

    const auto window_start = CurrentWindowStart();
    const auto key = BuildKey(user_id, window_start);

    try {
        // Incr с CommandControl
        auto count_result = redis_client_->Incr(key, {}).Get();
        std::int64_t count = count_result;

        // Если ключ создан только что, задаём TTL
        if (count == 1) {
            redis_client_->Expire(key, kWindowSize, {}).Get();  // фикс
        }

        decision.remaining = static_cast<std::uint32_t>(
            std::max(std::int64_t{0}, kBookingLimit - count));
        decision.reset_unix_seconds = window_start + kWindowSize.count();
        decision.allowed = count <= kBookingLimit;
    } catch (...) {
        // При недоступности Redis пропускаем запрос (fail open)
        decision.allowed = true;
        decision.remaining = kBookingLimit;
        decision.reset_unix_seconds = CurrentWindowStart() + kWindowSize.count();
    }

    return decision;
}
#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>

#include "cache.hpp"
#include "rate_limiter.hpp"

namespace handlers {

// --------------- Service Component ---------------

class HotelBookingServiceComponent final
    : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "hotel-booking-service";

    HotelBookingServiceComponent(const userver::components::ComponentConfig& config,
                                  const userver::components::ComponentContext& context);

    CacheService& GetCache() { return *cache_; }
    RateLimiter& GetRateLimiter() { return *rate_limiter_; }

private:
    std::unique_ptr<CacheService> cache_;
    std::unique_ptr<RateLimiter> rate_limiter_;
};

// --------------- Auth Context ---------------

struct AuthContext {
    std::int64_t user_id{0};
    std::string login;
};

// --------------- Handler Base ---------------

class HandlerBase : public userver::server::handlers::HttpHandlerBase {
public:
    HandlerBase(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);

protected:
    CacheService& GetCache() const;
    RateLimiter& GetRateLimiter() const;

    AuthContext RequireAuth(const userver::server::http::HttpRequest& request) const;

private:
    HotelBookingServiceComponent& service_;
};

// --------------- Concrete Handlers ---------------

class RegisterHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-auth-register";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class LoginHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-auth-login";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class UserByLoginHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-by-login";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class UserSearchHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-search";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class HotelsHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-hotels";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class BookingsHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-bookings";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class UserBookingsHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-user-bookings";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class BookingByIdHandler final : public HandlerBase {
public:
    static constexpr std::string_view kName = "handler-booking-by-id";
    using HandlerBase::HandlerBase;
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

}  // namespace handlers
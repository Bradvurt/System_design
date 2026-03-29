#pragma once

#include <string>
#include <string_view>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

namespace handlers {

class CreateUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-user";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class GetUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-get-user";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class CreateHotelHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-hotel";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class ListHotelsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-hotels";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class CreateBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-booking";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class ListUserBookingsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-user-bookings";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

class CancelBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cancel-booking";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest&,
                                   userver::server::request::RequestContext&) const override;
};

} // namespace handlers

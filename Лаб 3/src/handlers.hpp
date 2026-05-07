#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/component.hpp>

#include "models.hpp"

class Storage;

class PingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-ping";
    PingHandler(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
};

class CreateUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-user";
    CreateUserHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class GetUserHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-get-user";
    GetUserHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";
    LoginHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class CreateHotelHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-hotel";
    CreateHotelHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class ListHotelsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-hotels";
    ListHotelsHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class CreateBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-booking";
    CreateBookingHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class ListUserBookingsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-user-bookings";
    ListUserBookingsHandler(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};

class CancelBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cancel-booking";
    CancelBookingHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;
private:
    std::shared_ptr<Storage> storage_;
};
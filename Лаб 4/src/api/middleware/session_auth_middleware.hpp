#pragma once

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "components/hotel_booking_service_component.hpp"

namespace lab4 {

class SessionAuthMiddleware final : public userver::server::middlewares::HttpMiddlewareBase {
 public:
  SessionAuthMiddleware(
      const userver::server::handlers::HttpHandlerBase& handler,
      HotelBookingService& service);

 private:
  void HandleRequest(
      userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  bool RequiresAuth(const userver::server::http::HttpRequest& request) const;

  HotelBookingService& service_;
};

class SessionAuthMiddlewareFactory final
    : public userver::server::middlewares::HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName = "session-auth-middleware";

  SessionAuthMiddlewareFactory(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

 private:
  std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> Create(
      const userver::server::handlers::HttpHandlerBase& handler,
      userver::yaml_config::YamlConfig middleware_config) const override;

  HotelBookingService& service_;
};

}

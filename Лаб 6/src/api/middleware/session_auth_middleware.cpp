#include "api/middleware/session_auth_middleware.hpp"

#include <string>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/json_error_builder.hpp>

#include "api/api_utils.hpp"

namespace lab4 {

namespace {

userver::server::handlers::JsonErrorBuilder MakeUnauthorizedBody(
    std::string_view code,
    std::string_view message) {
  userver::formats::json::ValueBuilder body;
  body["error"]["code"] = std::string(code);
  body["error"]["message"] = std::string(message);
  const auto json = userver::formats::json::ToString(body.ExtractValue());
  return userver::server::handlers::JsonErrorBuilder{code, std::string(message), json};
}

bool StartsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool EndsWith(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size(), suffix.size()) == suffix;
}

}

SessionAuthMiddleware::SessionAuthMiddleware(
    const userver::server::handlers::HttpHandlerBase&,
    HotelBookingService& service)
    : service_(service) {}

bool SessionAuthMiddleware::RequiresAuth(const userver::server::http::HttpRequest& request) const {
  using userver::server::http::HttpMethod;

  const auto method = request.GetMethod();
  const auto& path = request.GetRequestPath();

  if (method == HttpMethod::kPost && path == "/api/v1/hotels") {
    return true;
  }
  if (method == HttpMethod::kPost && path == "/api/v1/bookings") {
    return true;
  }
  if (method == HttpMethod::kDelete && StartsWith(path, "/api/v1/bookings/")) {
    return true;
  }
  if (method == HttpMethod::kGet && StartsWith(path, "/api/v1/users/") && EndsWith(path, "/bookings")) {
    return true;
  }

  return false;
}

void SessionAuthMiddleware::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  if (!RequiresAuth(request)) {
    Next(request, context);
    return;
  }

  static constexpr std::string_view kBearerPrefix = "Bearer ";
  const auto& authorization = request.GetHeader("Authorization");
  if (!StartsWith(authorization, kBearerPrefix)) {
    throw userver::server::handlers::Unauthorized(
        MakeUnauthorizedBody("unauthorized", "Missing or invalid Authorization header"));
  }

  const std::string token(authorization.substr(kBearerPrefix.size()));
  const auto session = service_.Authenticate(token);
  if (!session.has_value()) {
    throw userver::server::handlers::Unauthorized(
        MakeUnauthorizedBody("unauthorized", "Authentication token is invalid"));
  }

  StoreAuthContext(context, AuthContext{session->user_id, session->login});
  Next(request, context);
}

SessionAuthMiddlewareFactory::SessionAuthMiddlewareFactory(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::middlewares::HttpMiddlewareFactoryBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase>
SessionAuthMiddlewareFactory::Create(
    const userver::server::handlers::HttpHandlerBase& handler,
    userver::yaml_config::YamlConfig) const {
  return std::make_unique<SessionAuthMiddleware>(handler, service_);
}

}

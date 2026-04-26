#include "api/handlers/auth_handlers.hpp"

#include <exception>

#include "api/api_utils.hpp"

namespace lab4 {

RegisterHandler::RegisterHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    const auto request_json = ParseJsonBody(request);
    const auto register_request = ParseRegisterUserRequest(request_json);
    const auto user = service_.RegisterUser(register_request);
    const auto session = service_.Login(LoginRequest{user.login, register_request.password});
    return MakeJsonResponse(
        request,
        userver::server::http::HttpStatus::kCreated,
        SessionToJson(session, user));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

LoginHandler::LoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    const auto request_json = ParseJsonBody(request);
    const auto login_request = ParseLoginRequest(request_json);
    const auto session = service_.Login(login_request);
    const auto user = service_.GetUserById(session.user_id);
    if (!user.has_value()) {
      return MakeInternalErrorResponse(request);
    }

    return MakeJsonResponse(
        request,
        userver::server::http::HttpStatus::kOk,
        SessionToJson(session, *user));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

}

#include "api/handlers/user_handlers.hpp"

#include <exception>

#include "api/api_utils.hpp"

namespace lab4 {

UserByLoginHandler::UserByLoginHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string UserByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    const auto& login = request.GetPathArg("login");
    const auto user = service_.FindUserByLogin(login);
    if (!user.has_value()) {
      return MakeErrorResponse(
          request,
          userver::server::http::HttpStatus::kNotFound,
          "user_not_found",
          "User was not found");
    }

    return MakeJsonResponse(request, userver::server::http::HttpStatus::kOk, UserToJson(*user));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

UserSearchHandler::UserSearchHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string UserSearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  try {
    const auto users = service_.SearchUsers(request.GetArg("name"), request.GetArg("surname"));
    return MakeJsonResponse(request, userver::server::http::HttpStatus::kOk, UsersToJson(users));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

}

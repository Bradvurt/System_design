#include "api/handlers/hotel_handlers.hpp"

#include <exception>
#include <optional>

#include "api/api_utils.hpp"

namespace lab4 {

HotelsHandler::HotelsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string HotelsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  try {
    using userver::server::http::HttpMethod;

    if (request.GetMethod() == HttpMethod::kGet) {
      std::optional<std::string> city;
      if (!request.GetArg("city").empty()) {
        city = request.GetArg("city");
      }

      const auto hotels = service_.ListHotels(city);
      return MakeJsonResponse(request, userver::server::http::HttpStatus::kOk, HotelsToJson(hotels));
    }

    const auto& auth = RequireAuthContext(context);
    const auto request_json = ParseJsonBody(request);
    const auto create_request = ParseCreateHotelRequest(request_json);
    const auto hotel = service_.CreateHotel(create_request, auth.user_id);
    return MakeJsonResponse(
        request,
        userver::server::http::HttpStatus::kCreated,
        HotelToJson(hotel));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

}

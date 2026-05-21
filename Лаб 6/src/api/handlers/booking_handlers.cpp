#include "api/handlers/booking_handlers.hpp"

#include <exception>

#include "api/api_utils.hpp"

namespace lab4 {

BookingsHandler::BookingsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string BookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  try {
    const auto& auth = RequireAuthContext(context);
    const auto request_json = ParseJsonBody(request);
    const auto create_request = ParseCreateBookingRequest(request_json);
    const auto booking = service_.CreateBooking(create_request, auth.user_id);
    return MakeJsonResponse(
        request,
        userver::server::http::HttpStatus::kCreated,
        BookingToJson(booking));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

UserBookingsHandler::UserBookingsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string UserBookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  try {
    const auto& auth = RequireAuthContext(context);
    const auto user_id = ParseId(request.GetPathArg("user_id"), "user_id");
    const auto bookings = service_.ListUserBookings(auth.user_id, user_id);
    return MakeJsonResponse(
        request,
        userver::server::http::HttpStatus::kOk,
        BookingsToJson(bookings));
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

BookingByIdHandler::BookingByIdHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>().GetService()) {}

std::string BookingByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  try {
    const auto& auth = RequireAuthContext(context);
    const auto booking_id = ParseId(request.GetPathArg("booking_id"), "booking_id");
    service_.CancelBooking(auth.user_id, booking_id);
    return MakeNoContentResponse(request, userver::server::http::HttpStatus::kNoContent);
  } catch (const ServiceError& ex) {
    return MakeErrorResponse(request, ToHttpStatus(ex.GetKind()), ex.GetCode(), ex.what());
  } catch (const std::exception&) {
    return MakeInternalErrorResponse(request);
  }
}

}

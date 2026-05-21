#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "components/hotel_booking_service_component.hpp"

namespace lab4 {

class BookingsHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-bookings";

  BookingsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

 private:
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  HotelBookingService& service_;
};

class UserBookingsHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user-bookings";

  UserBookingsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

 private:
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  HotelBookingService& service_;
};

class BookingByIdHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-booking-by-id";

  BookingByIdHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

 private:
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  HotelBookingService& service_;
};

}

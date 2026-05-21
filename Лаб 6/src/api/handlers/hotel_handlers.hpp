#pragma once

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include "components/hotel_booking_service_component.hpp"

namespace lab4 {

class HotelsHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-hotels";

  HotelsHandler(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context);

 private:
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& context) const override;

  HotelBookingService& service_;
};

}

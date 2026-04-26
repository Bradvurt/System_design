#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/mongo/component.hpp>

#include "domain/service.hpp"

namespace lab4 {

class HotelBookingServiceComponent final : public userver::components::ComponentBase {
 public:
  static constexpr std::string_view kName = "hotel-booking";

  HotelBookingServiceComponent(
      const userver::components::ComponentConfig& config,
      const userver::components::ComponentContext& context)
      : userver::components::ComponentBase(config, context),
        service_(
            context.FindComponent<userver::components::Mongo>("mongo-db").GetPool()) {}

  HotelBookingService& GetService() { return service_; }
  const HotelBookingService& GetService() const { return service_; }

 private:
  HotelBookingService service_;
};

} // namespace lab4
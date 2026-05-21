#pragma once
#include <userver/components/component_base.hpp>
#include <userver/urabbitmq/client.hpp>
#include "domain/models.hpp"
#include <memory>
#include <string>

namespace booking {

class EventPublisherComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "event-publisher";

    EventPublisherComponent(const userver::components::ComponentConfig& config,
                            const userver::components::ComponentContext& context);

    void PublishUserRegistered(const User& user) const;
    void PublishHotelCreated(const Hotel& hotel) const;
    void PublishBookingCreated(const Booking& booking) const;
    void PublishBookingCancelled(std::int64_t booking_id, std::int64_t user_id) const;

private:
    void PublishMessage(const std::string& routing_key, const std::string& message) const;
    std::shared_ptr<userver::urabbitmq::Client> client_;
    static constexpr std::string_view kExchangeName = "booking_events";
};

} // namespace booking
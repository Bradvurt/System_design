#include "events/event_publisher.hpp"
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/engine/deadline.hpp>

namespace booking {

EventPublisherComponent::EventPublisherComponent(const userver::components::ComponentConfig& config,
                                                 const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      client_(context.FindComponent<userver::urabbitmq::Client>("booking-rabbitmq").GetClient()) {
    // Declare exchange on startup
    try {
        auto admin = client_->GetAdminChannel(userver::engine::Deadline::FromDuration(std::chrono::seconds{10}));
        admin.DeclareExchange(userver::urabbitmq::Exchange{std::string(kExchangeName)},
                              userver::urabbitmq::Exchange::Type::kTopic, {},
                              userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
        LOG_INFO() << "EventPublisher: Exchange '" << kExchangeName << "' declared";
    } catch (const std::exception& e) {
        LOG_WARNING() << "EventPublisher: Exchange declaration failed: " << e.what();
    }
}

void EventPublisherComponent::PublishMessage(const std::string& routing_key,
                                            const std::string& message) const {
    try {
        client_->Publish(userver::urabbitmq::Exchange{std::string(kExchangeName)},
                         routing_key, message,
                         userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
        LOG_INFO() << "Event published: " << routing_key;
    } catch (const std::exception& e) {
        LOG_WARNING() << "Failed to publish event " << routing_key << ": " << e.what();
    }
}

void EventPublisherComponent::PublishUserRegistered(const User& user) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = "user.registered";
    payload["user_id"] = user.id;
    payload["login"] = user.login;
    payload["email"] = user.email;
    PublishMessage("user.registered", userver::formats::json::ToString(payload.ExtractValue()));
}

void EventPublisherComponent::PublishHotelCreated(const Hotel& hotel) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = "hotel.created";
    payload["hotel_id"] = hotel.id;
    payload["name"] = hotel.name;
    payload["city"] = hotel.city;
    payload["stars"] = hotel.stars;
    PublishMessage("hotel.created", userver::formats::json::ToString(payload.ExtractValue()));
}

void EventPublisherComponent::PublishBookingCreated(const Booking& booking) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = "booking.created";
    payload["booking_id"] = std::to_string(booking.id); // Assuming UUID or string ID
    payload["user_id"] = booking.user_id;
    payload["hotel_id"] = booking.hotel_id;
    payload["check_in"] = booking.check_in;
    payload["check_out"] = booking.check_out;
    payload["status"] = ToString(booking.status);
    PublishMessage("booking.created", userver::formats::json::ToString(payload.ExtractValue()));
}

void EventPublisherComponent::PublishBookingCancelled(std::int64_t booking_id, std::int64_t user_id) const {
    userver::formats::json::ValueBuilder payload;
    payload["event_type"] = "booking.cancelled";
    payload["booking_id"] = std::to_string(booking_id);
    payload["user_id"] = user_id;
    PublishMessage("booking.cancelled", userver::formats::json::ToString(payload.ExtractValue()));
}

} // namespace booking
#include "events/mailer_consumer.hpp"
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/engine/deadline.hpp>

namespace booking {

MailerConsumer::MailerConsumer(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : ConsumerComponentBase(config, context) {
    // Setup exchange, queue, and binding
    auto& rmq = context.FindComponent<userver::urabbitmq::Client>("booking-rabbitmq");
    auto client = rmq.GetClient();
    auto deadline = userver::engine::Deadline::FromDuration(std::chrono::seconds{10});
    auto admin = client->GetAdminChannel(deadline);

    admin.DeclareExchange(userver::urabbitmq::Exchange{"booking_events"},
                          userver::urabbitmq::Exchange::Type::kTopic, {},
                          userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
    admin.DeclareQueue(userver::urabbitmq::Queue{"mailer_queue"}, {},
                       userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
    admin.BindQueue(userver::urabbitmq::Exchange{"booking_events"},
                    userver::urabbitmq::Queue{"mailer_queue"},
                    "booking.*",
                    userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));
    admin.BindQueue(userver::urabbitmq::Exchange{"booking_events"},
                    userver::urabbitmq::Queue{"mailer_queue"},
                    "user.registered",
                    userver::engine::Deadline::FromDuration(std::chrono::seconds{5}));

    LOG_INFO() << "MailerConsumer: Exchange, queue, and bindings set up";
}

void MailerConsumer::Process(std::string message) {
    LOG_INFO() << "Received event: " << message;
    try {
        auto json = userver::formats::json::FromString(message);
        auto event_type = json["event_type"].As<std::string>("");

        if (event_type == "user.registered") {
            auto login = json["login"].As<std::string>("");
            auto email = json["email"].As<std::string>("");
            LOG_INFO() << "[MAILER] Sending welcome email to " << login << " at " << email;
        } else if (event_type == "booking.created") {
            auto booking_id = json["booking_id"].As<std::string>("");
            auto user_id = json["user_id"].As<std::int64_t>(0);
            auto hotel_id = json["hotel_id"].As<std::int64_t>(0);
            auto check_in = json["check_in"].As<std::string>("");
            auto check_out = json["check_out"].As<std::string>("");
            LOG_INFO() << "[MAILER] Sending booking confirmation | booking=" << booking_id
                       << " user=" << user_id << " hotel=" << hotel_id
                       << " dates=" << check_in << "/" << check_out;
        } else if (event_type == "booking.cancelled") {
            auto booking_id = json["booking_id"].As<std::string>("");
            auto user_id = json["user_id"].As<std::int64_t>(0);
            LOG_INFO() << "[MAILER] Sending cancellation email | booking=" << booking_id
                       << " user=" << user_id;
        } else {
            LOG_WARNING() << "[MAILER] Unknown event type: " << event_type;
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "[MAILER] Failed to process event: " << e.what();
    }
}

} // namespace booking
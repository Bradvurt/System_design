#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_main.hpp>

#include "components/hotel_booking_service_component.hpp"
#include "events/event_publisher.hpp"
#include "events/mailer_consumer.hpp"
#include "api/handlers/auth_handlers.hpp"
#include "api/handlers/user_handlers.hpp"
#include "api/handlers/hotel_handlers.hpp"
#include "api/handlers/booking_handlers.hpp"
#include "api/middleware/auth_middleware.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<booking::HotelBookingServiceComponent>()
        .Append<booking::EventPublisherComponent>()
        .Append<booking::MailerConsumer>()
        .Append<booking::RegisterHandler>()
        .Append<booking::LoginHandler>()
        .Append<booking::UserByLoginHandler>()
        .Append<booking::UserSearchHandler>()
        .Append<booking::CreateHotelHandler>()
        .Append<booking::ListHotelsHandler>()
        .Append<booking::BookingsHandler>()
        .Append<booking::UserBookingsHandler>()
        .Append<booking::BookingByIdHandler>()
        .Append<booking::AuthMiddleware>()
        .Append<userver::components::Mongo>("mongo-db")
        .Append<userver::urabbitmq::Client>("booking-rabbitmq");

    return userver::utils::DaemonMain(argc, argv, component_list);
}
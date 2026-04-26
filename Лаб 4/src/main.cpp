#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "components/hotel_booking_service_component.hpp"
#include "api/handlers/auth_handlers.hpp"
#include "api/handlers/booking_handlers.hpp"
#include "api/handlers/hotel_handlers.hpp"
#include "api/handlers/user_handlers.hpp"
#include "api/middleware/session_auth_middleware.hpp"

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<lab4::HotelBookingServiceComponent>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::Mongo>("mongo-db")
          .Append<lab4::SessionAuthMiddlewareFactory>()
          .Append<lab4::RegisterHandler>()
          .Append<lab4::LoginHandler>()
          .Append<lab4::UserByLoginHandler>()
          .Append<lab4::UserSearchHandler>()
          .Append<lab4::HotelsHandler>()
          .Append<lab4::BookingsHandler>()
          .Append<lab4::UserBookingsHandler>()
          .Append<lab4::BookingByIdHandler>();

  return userver::utils::DaemonMain(argc, argv, component_list);
}

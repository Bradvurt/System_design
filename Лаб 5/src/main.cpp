#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::components::TestsuiteSupport>()   // bruh
            .Append<userver::components::Redis>("redis-cache")
            .Append<handlers::RegisterHandler>()
            .Append<handlers::LoginHandler>()
            .Append<handlers::UserByLoginHandler>()
            .Append<handlers::UserSearchHandler>()
            .Append<handlers::HotelsHandler>()
            .Append<handlers::BookingsHandler>()
            .Append<handlers::UserBookingsHandler>()
            .Append<handlers::BookingByIdHandler>()
            .Append<handlers::HotelBookingServiceComponent>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
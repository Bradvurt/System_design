#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "auth.hpp"
#include "handlers.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<SessionAuthCheckerFactory>();

    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<userver::components::TestsuiteSupport>("testsuite-support")
        .Append<userver::clients::dns::Component>("dns-client")
        .Append<PingHandler>()
        .Append<CreateUserHandler>()
        .Append<GetUserHandler>()
        .Append<LoginHandler>()
        .Append<CreateHotelHandler>()
        .Append<ListHotelsHandler>()
        .Append<CreateBookingHandler>()
        .Append<ListUserBookingsHandler>()
        .Append<CancelBookingHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
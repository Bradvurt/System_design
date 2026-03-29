#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers.hpp"
#include "auth.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<auth::JwtAuthCheckerFactory>();

    auto component_list = userver::components::MinimalServerComponentList();
    component_list.Append<userver::server::handlers::Ping>();
    component_list.Append<userver::server::handlers::TestsControl>();
    component_list.Append<userver::server::handlers::ServerMonitor>();

    component_list.Append<handlers::CreateUserHandler>();
    component_list.Append<handlers::GetUserHandler>();
    component_list.Append<handlers::LoginHandler>();
    component_list.Append<handlers::CreateHotelHandler>();
    component_list.Append<handlers::ListHotelsHandler>();
    component_list.Append<handlers::CreateBookingHandler>();
    component_list.Append<handlers::ListUserBookingsHandler>();
    component_list.Append<handlers::CancelBookingHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}

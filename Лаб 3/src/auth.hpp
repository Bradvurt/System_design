#pragma once

#include <string>
#include <memory>

#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>

class Storage;

class SessionAuthCheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "session";

    explicit SessionAuthCheckerFactory(
        const userver::components::ComponentContext& context);

    userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const userver::server::handlers::auth::HandlerAuthConfig& auth_config)
        const override;

private:
    std::shared_ptr<Storage> storage_;
};
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/yaml_config/schema.hpp>

namespace auth {

std::string GenerateToken(int user_id);
std::string GenerateToken(int user_id, const std::string& secret);
std::optional<int> ValidateToken(const std::string& token, const std::string& secret);

std::string HashPassword(const std::string& password);
bool VerifyPassword(const std::string& password, const std::string& hash);

class JwtAuthCheckerFactory final : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "jwt";

    explicit JwtAuthCheckerFactory(const userver::components::ComponentContext& context);

    userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const userver::server::handlers::auth::HandlerAuthConfig& auth_config) const override;

private:
    std::string secret_;
};

} // namespace auth

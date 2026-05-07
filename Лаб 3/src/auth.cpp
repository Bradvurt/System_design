#include "auth.hpp"

#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/component.hpp>  // <-- обязательно для Postgres
#include <userver/storages/postgres/cluster.hpp>

#include "models.hpp"

namespace {

class SessionAuthChecker final
    : public userver::server::handlers::auth::AuthCheckerBase {
public:
    explicit SessionAuthChecker(std::shared_ptr<Storage> storage)
        : storage_(std::move(storage)) {}

    userver::server::handlers::auth::AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& request_context)
        const override {
        using Status = userver::server::handlers::auth::AuthCheckResult::Status;

        // Используем строковый литерал "Authorization"
        const auto& auth_header = request.GetHeader("Authorization");
        if (auth_header.empty()) {
            return {Status::kTokenNotFound, {},
                    "Empty 'Authorization' header",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        constexpr std::string_view kBearerPrefix = "Bearer ";
        if (auth_header.size() <= kBearerPrefix.size() ||
            auth_header.compare(0, kBearerPrefix.size(), kBearerPrefix) != 0) {
            return {Status::kTokenNotFound, {},
                    "'Authorization' header should have 'Bearer some-token' format",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        const std::string token = auth_header.substr(kBearerPrefix.size());
        const auto session = storage_->Authenticate(token);
        if (!session.has_value()) {
            return {Status::kInvalidToken, {},
                    "Invalid or expired session token",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        request_context.SetData("user_id", session->user_id);
        return {};
    }

    // Реализация чисто виртуальной функции
    [[nodiscard]] bool SupportsUserAuth() const noexcept override {
        return false;   // у нас сессионная аутентификация, не пользовательская
    }

private:
    std::shared_ptr<Storage> storage_;
};

} // namespace

SessionAuthCheckerFactory::SessionAuthCheckerFactory(
    const userver::components::ComponentContext& context) {
    // Полное определение Postgres доступно благодаря включённому component.hpp
    auto& pg = context.FindComponent<userver::components::Postgres>("postgres-db");
    storage_ = std::make_shared<Storage>(pg.GetCluster());
}

userver::server::handlers::auth::AuthCheckerBasePtr
SessionAuthCheckerFactory::MakeAuthChecker(
    const userver::server::handlers::auth::HandlerAuthConfig& /*auth_config*/)
    const {
    return std::make_unique<SessionAuthChecker>(storage_);
}
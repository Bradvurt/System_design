#include "auth.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/handler_config.hpp>

namespace {

constexpr std::string_view kJwtSecret = "lab2-super-secret-key-change-me-123";
constexpr std::string_view kIssuer = "hotel-booking";

std::string Base64UrlEncode(const unsigned char* data, std::size_t len) {
    std::string encoded(4 * ((len + 2) / 3) + 1, '\0');
    const int out_len = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encoded.data()), data, static_cast<int>(len));
    encoded.resize(static_cast<std::size_t>(out_len));

    for (char& ch : encoded) {
        if (ch == '+') ch = '-';
        else if (ch == '/') ch = '_';
    }
    while (!encoded.empty() && encoded.back() == '=') encoded.pop_back();
    return encoded;
}

std::optional<std::string> Base64UrlDecode(std::string input) {
    for (char& ch : input) {
        if (ch == '-') ch = '+';
        else if (ch == '_') ch = '/';
    }

    while (input.size() % 4 != 0) input.push_back('=');

    std::string decoded(input.size() * 3 / 4 + 1, '\0');
    const int out_len = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(decoded.data()),
                                        reinterpret_cast<const unsigned char*>(input.data()),
                                        static_cast<int>(input.size()));
    if (out_len < 0) return std::nullopt;

    std::size_t padding = 0;
    if (!input.empty() && input.back() == '=') padding++;
    if (input.size() > 1 && input[input.size() - 2] == '=') padding++;
    if (static_cast<std::size_t>(out_len) < padding) return std::nullopt;
    decoded.resize(static_cast<std::size_t>(out_len) - padding);
    return decoded;
}

std::string HmacSha256Base64Url(const std::string& secret, const std::string& data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;
    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         result, &result_len);
    return Base64UrlEncode(result, result_len);
}

} // namespace

namespace auth {

std::string GenerateToken(int user_id) {
    return GenerateToken(user_id, std::string(kJwtSecret));
}

std::string GenerateToken(int user_id, const std::string& secret) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto expires = now + hours(24);
    const auto now_sec = duration_cast<seconds>(now.time_since_epoch()).count();
    const auto exp_sec = duration_cast<seconds>(expires.time_since_epoch()).count();

    const std::string header = std::string("{\"alg\":\"HS256\",\"typ\":\"JWT\"}");
    const std::string payload = std::string("{") +
                                "\"iss\":\"" + std::string(kIssuer) + "\"," +
                                "\"user_id\":" + std::to_string(user_id) + "," +
                                "\"iat\":" + std::to_string(now_sec) + "," +
                                "\"exp\":" + std::to_string(exp_sec) +
                                "}";

    const std::string header64 = Base64UrlEncode(reinterpret_cast<const unsigned char*>(header.data()), header.size());
    const std::string payload64 = Base64UrlEncode(reinterpret_cast<const unsigned char*>(payload.data()), payload.size());
    const std::string signing_input = header64 + "." + payload64;
    const std::string signature64 = HmacSha256Base64Url(secret, signing_input);
    return signing_input + "." + signature64;
}

std::optional<int> ValidateToken(const std::string& token, const std::string& secret) {
    const auto first_dot = token.find('.');
    const auto second_dot = token.find('.', first_dot == std::string::npos ? first_dot : first_dot + 1);
    if (first_dot == std::string::npos || second_dot == std::string::npos) return std::nullopt;

    const std::string header64 = token.substr(0, first_dot);
    const std::string payload64 = token.substr(first_dot + 1, second_dot - first_dot - 1);
    const std::string signature64 = token.substr(second_dot + 1);

    const std::string signing_input = header64 + "." + payload64;
    const std::string expected_signature64 = HmacSha256Base64Url(secret, signing_input);
    if (signature64.size() != expected_signature64.size() ||
        CRYPTO_memcmp(signature64.data(), expected_signature64.data(), signature64.size()) != 0) {
        return std::nullopt;
    }

    const auto payload_json = Base64UrlDecode(payload64);
    if (!payload_json) return std::nullopt;

    try {
        const auto payload = userver::formats::json::FromString(*payload_json);
        const auto user_id = payload["user_id"].As<int>();
        const auto exp = payload["exp"].As<long long>();
        const auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now_sec > exp) return std::nullopt;
        return user_id;
    } catch (...) {
        return std::nullopt;
    }
}

std::string HashPassword(const std::string& password) {
    return password;
}

bool VerifyPassword(const std::string& password, const std::string& hash) {
    return password == hash;
}

JwtAuthCheckerFactory::JwtAuthCheckerFactory(const userver::components::ComponentContext&)
    : secret_(std::string(kJwtSecret)) {}

class JwtAuthChecker final : public userver::server::handlers::auth::AuthCheckerBase {
public:
    explicit JwtAuthChecker(std::string secret)
        : secret_(std::move(secret)) {}

    userver::server::handlers::auth::AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& request_context) const override {
        const auto& auth_header = request.GetHeader(userver::http::headers::kAuthorization);
        if (auth_header.empty()) {
            return {userver::server::handlers::auth::AuthCheckResult::Status::kTokenNotFound,
                    {}, "Empty 'Authorization' header",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        constexpr std::string_view kBearerPrefix = "Bearer ";
        if (auth_header.size() <= kBearerPrefix.size() || auth_header.compare(0, kBearerPrefix.size(), kBearerPrefix) != 0) {
            return {userver::server::handlers::auth::AuthCheckResult::Status::kTokenNotFound,
                    {}, "'Authorization' header should have 'Bearer some-token' format",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        const std::string token = auth_header.substr(kBearerPrefix.size());
        const auto user_id = auth::ValidateToken(token, secret_);
        if (!user_id) {
            return {userver::server::handlers::auth::AuthCheckResult::Status::kInvalidToken,
                    {}, "Invalid or expired token",
                    userver::server::handlers::HandlerErrorCode::kUnauthorized};
        }

        request_context.SetData("user_id", *user_id);
        return {};
    }

    bool SupportsUserAuth() const noexcept override { return true; }

private:
    std::string secret_;
};

userver::server::handlers::auth::AuthCheckerBasePtr JwtAuthCheckerFactory::MakeAuthChecker(
    const userver::server::handlers::auth::HandlerAuthConfig&) const {
    return std::make_shared<JwtAuthChecker>(secret_);
}

} // namespace auth

#include "auth.hpp"
#include <userver/crypto/hash.hpp>
#include <userver/crypto/jwt.hpp>
#include <userver/utils/datetime.hpp>

namespace auth {

std::string HashPassword(const std::string& password) {
    return userver::crypto::hash::Sha256(password);
}

bool VerifyPassword(const std::string& password, const std::string& hash) {
    return HashPassword(password) == hash;
}

std::string GenerateToken(int user_id) {
    userver::crypto::jwt::JwtBuilder builder;
    builder.SetSubject(std::to_string(user_id));
    builder.SetExpirationTime(userver::utils::datetime::Now() +
                              std::chrono::hours(24));
    return builder.Build();
}

int VerifyToken(const std::string& token) {
    auto jwt = userver::crypto::jwt::Parse(token);
    return std::stoi(jwt.GetSubject());
}

} // namespace auth
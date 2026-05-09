#include "auth.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace auth {

namespace {

// FNV-1a 64-bit hash
std::string Fnv1aHex(std::string_view value) {
    constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t hash = kOffsetBasis;
    for (unsigned char ch : value) {
        hash ^= ch;
        hash *= kPrime;
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0') << std::setw(16) << hash;
    return out.str();
}

}  // namespace

std::string GenerateToken(std::int64_t user_id, const std::string& login) {
    const auto seed =
        login + ":" + std::to_string(user_id) + ":" + Fnv1aHex(login);
    return "session-" + Fnv1aHex(seed);
}

std::string HashPassword(const std::string& password) {
    return Fnv1aHex("password:" + password);
}

bool VerifyPassword(const std::string& password, const std::string& hash) {
    return HashPassword(password) == hash;
}

}  // namespace auth
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace auth {

// Generate a simple session token for a user
std::string GenerateToken(std::int64_t user_id, const std::string& login);

// Hash a password using FNV-1a
std::string HashPassword(const std::string& password);

// Verify password against hash
bool VerifyPassword(const std::string& password, const std::string& hash);

}  // namespace auth
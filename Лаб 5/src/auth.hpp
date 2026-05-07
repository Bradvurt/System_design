#pragma once

#include <string>

namespace auth {

std::string HashPassword(const std::string& password);
bool VerifyPassword(const std::string& password, const std::string& hash);
std::string GenerateToken(int user_id);
int VerifyToken(const std::string& token);

} // namespace auth
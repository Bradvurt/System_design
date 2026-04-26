#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

std::string Exec(const std::string& cmd) {
  std::array<char, 4096> buffer{};
  std::string result;

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) throw std::runtime_error("Failed to execute command");

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
    result += buffer.data();
  }
  const int rc = pclose(pipe);
  Check(rc == 0, "Command failed: " + cmd);
  return result;
}

std::string EnvOrDefault(const char* name, const std::string& fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr || std::string(value).empty()) return fallback;
  return value;
}

bool Contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

std::string ExtractStringField(const std::string& json, const std::string& field_name) {
  const auto key = "\"" + field_name + "\":\"";
  const auto start = json.find(key);
  if (start == std::string::npos) return {};
  const auto value_start = start + key.size();
  const auto value_end = json.find('"', value_start);
  if (value_end == std::string::npos) return {};
  return json.substr(value_start, value_end - value_start);
}

std::string ExtractNumberField(const std::string& json, const std::string& field_name) {
  const auto key = "\"" + field_name + "\":";
  const auto start = json.find(key);
  if (start == std::string::npos) return {};
  auto pos = start + key.size();
  while (pos < json.size() && json[pos] == ' ') ++pos;
  const auto end = json.find_first_not_of("0123456789", pos);
  if (end == std::string::npos) return json.substr(pos);
  return json.substr(pos, end - pos);
}

void TestApiSmoke() {
  const auto base_url = EnvOrDefault("LAB4_BASE_URL", "http://localhost:8080");

  const std::string login = "service-test-user";
  const std::string email = "service-test-user@example.com";

  const auto register_cmd =
      "curl -sS -X POST " + base_url +
      "/api/v1/auth/register -H \"Content-Type: application/json\" "
      "-d '{\"login\":\"" + login +
      "\",\"name\":\"Service\",\"surname\":\"Test\",\"email\":\"" + email +
      "\",\"password\":\"secret123\"}'";
  const auto register_resp = Exec(register_cmd);
  Check(Contains(register_resp, "\"token\""), "register response has no token");
  const auto token = ExtractStringField(register_resp, "token");
  const auto user_id = ExtractNumberField(register_resp, "id");
  Check(!token.empty(), "token parsing failed");
  Check(!user_id.empty(), "user_id parsing failed");

  const auto by_login_resp =
      Exec("curl -sS " + base_url + "/api/v1/users/by-login/" + login);
  Check(Contains(by_login_resp, "\"login\":\"" + login + "\""), "user lookup by login failed");

  const auto search_resp =
      Exec("curl -sS \"" + base_url + "/api/v1/users/search?name=Service&surname=Test\"");
  Check(Contains(search_resp, "\"login\":\"" + login + "\""), "user search failed");

  const auto create_hotel_resp =
      Exec("curl -sS -X POST " + base_url +
           "/api/v1/hotels -H \"Content-Type: application/json\" "
           "-H \"Authorization: Bearer " + token + "\" "
           "-d '{\"name\":\"Service Test Hotel\",\"city\":\"Kazan\",\"address\":\"Test st 1\","
           "\"description\":\"integration\"}'");
  const auto hotel_id = ExtractNumberField(create_hotel_resp, "id");
  Check(!hotel_id.empty(), "hotel creation failed");

  const auto hotels_resp = Exec("curl -sS \"" + base_url + "/api/v1/hotels?city=Kazan\"");
  Check(Contains(hotels_resp, "\"city\":\"Kazan\""), "city hotel search failed");

  const auto create_booking_resp =
      Exec("curl -sS -X POST " + base_url +
           "/api/v1/bookings -H \"Content-Type: application/json\" "
           "-H \"Authorization: Bearer " + token + "\" "
           "-d '{\"hotel_id\":" + hotel_id +
           ",\"check_in\":\"2026-08-01\",\"check_out\":\"2026-08-03\"}'");
  const auto booking_id = ExtractNumberField(create_booking_resp, "id");
  Check(!booking_id.empty(), "booking creation failed");

  const auto user_bookings_resp =
      Exec("curl -sS \"" + base_url + "/api/v1/users/" + user_id +
           "/bookings\" -H \"Authorization: Bearer " + token + "\"");
  Check(Contains(user_bookings_resp, "\"id\":" + booking_id), "user bookings fetch failed");

  const auto cancel_status = Exec(
      "curl -sS -o /dev/null -w \"%{http_code}\" -X DELETE " + base_url + "/api/v1/bookings/" +
      booking_id + " -H \"Authorization: Bearer " + token + "\"");
  Check(Contains(cancel_status, "204"), "cancel booking did not return 204");
}

}  // namespace

int main() {
  try {
    TestApiSmoke();
    std::cout << "All service tests passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}

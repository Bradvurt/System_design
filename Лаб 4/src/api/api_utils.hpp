#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include "domain/models.hpp"

namespace lab4 {

inline constexpr std::string_view kAuthContextKey = "auth-context";

userver::formats::json::Value ParseJsonBody(const userver::server::http::HttpRequest& request);

std::string MakeJsonResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status,
    const userver::formats::json::Value& body);

std::string MakeErrorResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status,
    std::string_view code,
    std::string_view message);

std::string MakeNoContentResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status);

std::string MakeInternalErrorResponse(const userver::server::http::HttpRequest& request);

userver::server::http::HttpStatus ToHttpStatus(ServiceErrorKind kind);
std::int64_t ParseId(std::string_view value, std::string_view field_name);

RegisterUserRequest ParseRegisterUserRequest(const userver::formats::json::Value& json);
LoginRequest ParseLoginRequest(const userver::formats::json::Value& json);
CreateHotelRequest ParseCreateHotelRequest(const userver::formats::json::Value& json);
CreateBookingRequest ParseCreateBookingRequest(const userver::formats::json::Value& json);

userver::formats::json::Value UserToJson(const User& user);
userver::formats::json::Value SessionToJson(const Session& session, const User& user);
userver::formats::json::Value RoomToJson(const Room& room);
userver::formats::json::Value HotelToJson(const Hotel& hotel);
userver::formats::json::Value BookingToJson(const Booking& booking);
userver::formats::json::Value UsersToJson(const std::vector<User>& users);
userver::formats::json::Value HotelsToJson(const std::vector<Hotel>& hotels);
userver::formats::json::Value BookingsToJson(const std::vector<Booking>& bookings);

void StoreAuthContext(userver::server::request::RequestContext& context, const AuthContext& auth_context);
const AuthContext& RequireAuthContext(const userver::server::request::RequestContext& context);

}
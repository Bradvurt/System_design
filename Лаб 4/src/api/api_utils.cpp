#include "api/api_utils.hpp"

#include <exception>
#include <string>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>

namespace lab4 {

namespace {

std::string RequireStringField(const userver::formats::json::Value& json, std::string_view name) {
  const auto field = json[name];
  if (field.IsMissing()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Missing field '" + std::string(name) + "'");
  }
  try {
    return field.As<std::string>();
  } catch (const std::exception&) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Field '" + std::string(name) + "' must be a string");
  }
}

std::int64_t RequireInt64Field(const userver::formats::json::Value& json, std::string_view name) {
  const auto field = json[name];
  if (field.IsMissing()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Missing field '" + std::string(name) + "'");
  }
  try {
    return field.As<std::int64_t>();
  } catch (const std::exception&) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Field '" + std::string(name) + "' must be an integer");
  }
}

std::string OptionalStringField(const userver::formats::json::Value& json, std::string_view name) {
  const auto field = json[name];
  if (field.IsMissing()) {
    return {};
  }
  try {
    return field.As<std::string>();
  } catch (const std::exception&) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Field '" + std::string(name) + "' must be a string");
  }
}

}

userver::formats::json::Value ParseJsonBody(const userver::server::http::HttpRequest& request) {
  try {
    return userver::formats::json::FromString(request.RequestBody());
  } catch (const std::exception&) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_json",
        "Request body must be a valid JSON document");
  }
}

std::string MakeJsonResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status,
    const userver::formats::json::Value& body) {
  request.SetResponseStatus(status);
  request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
  return userver::formats::json::ToString(body);
}

std::string MakeErrorResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status,
    std::string_view code,
    std::string_view message) {
  userver::formats::json::ValueBuilder builder;
  builder["error"]["code"] = std::string(code);
  builder["error"]["message"] = std::string(message);
  return MakeJsonResponse(request, status, builder.ExtractValue());
}

std::string MakeNoContentResponse(
    const userver::server::http::HttpRequest& request,
    userver::server::http::HttpStatus status) {
  request.SetResponseStatus(status);
  return {};
}

std::string MakeInternalErrorResponse(const userver::server::http::HttpRequest& request) {
  return MakeErrorResponse(
      request,
      userver::server::http::HttpStatus::kInternalServerError,
      "internal_error",
      "Internal server error");
}

userver::server::http::HttpStatus ToHttpStatus(ServiceErrorKind kind) {
  using Status = userver::server::http::HttpStatus;
  switch (kind) {
    case ServiceErrorKind::kBadRequest:
      return Status::kBadRequest;
    case ServiceErrorKind::kUnauthorized:
      return Status::kUnauthorized;
    case ServiceErrorKind::kForbidden:
      return Status::kForbidden;
    case ServiceErrorKind::kNotFound:
      return Status::kNotFound;
    case ServiceErrorKind::kConflict:
      return Status::kConflict;
  }
  return Status::kInternalServerError;
}

std::int64_t ParseId(std::string_view value, std::string_view field_name) {
  try {
    std::size_t parsed_length = 0;
    const auto parsed = std::stoll(std::string(value), &parsed_length);
    if (parsed_length != value.size()) {
      throw std::invalid_argument("invalid integer");
    }
    return parsed;
  } catch (const std::exception&) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "Field '" + std::string(field_name) + "' must be a valid integer");
  }
}

RegisterUserRequest ParseRegisterUserRequest(const userver::formats::json::Value& json) {
  RegisterUserRequest request;
  request.login = RequireStringField(json, "login");
  request.name = RequireStringField(json, "name");
  request.surname = RequireStringField(json, "surname");
  request.email = RequireStringField(json, "email");
  request.password = RequireStringField(json, "password");
  return request;
}

LoginRequest ParseLoginRequest(const userver::formats::json::Value& json) {
  LoginRequest request;
  request.login = RequireStringField(json, "login");
  request.password = RequireStringField(json, "password");
  return request;
}

CreateHotelRequest ParseCreateHotelRequest(const userver::formats::json::Value& json) {
  CreateHotelRequest request;
  request.name = RequireStringField(json, "name");
  request.city = RequireStringField(json, "city");
  request.address = RequireStringField(json, "address");
  request.description = OptionalStringField(json, "description");
  
  // Парсим номера если они есть
  if (json.HasMember("rooms") && json["rooms"].IsArray()) {
    for (const auto& room_json : json["rooms"]) {
      Room room;
      room.room_number = RequireStringField(room_json, "room_number");
      room.type = RequireStringField(room_json, "type");
      room.price_per_night = room_json["price_per_night"].As<double>();
      room.capacity = static_cast<int>(room_json["capacity"].As<std::int64_t>());
      room.is_available = room_json["is_available"].As<bool>(true);
      room.checkout_date = OptionalStringField(room_json, "checkout_date");
      request.rooms.push_back(std::move(room));
    }
  }
  
  return request;
}

CreateBookingRequest ParseCreateBookingRequest(const userver::formats::json::Value& json) {
  CreateBookingRequest request;
  request.hotel_id = RequireInt64Field(json, "hotel_id");
  request.room_number = RequireStringField(json, "room_number");
  request.check_in = RequireStringField(json, "check_in");
  request.check_out = RequireStringField(json, "check_out");
  return request;
}

userver::formats::json::Value UserToJson(const User& user) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = user.id;
  builder["login"] = user.login;
  builder["first_name"] = user.first_name;
  builder["last_name"] = user.last_name;
  builder["email"] = user.email;
  if (!user.phone.empty()) {
    builder["phone"] = user.phone;
  }
  return builder.ExtractValue();
}

userver::formats::json::Value SessionToJson(const Session& session, const User& user) {
  userver::formats::json::ValueBuilder builder;
  builder["token"] = session.token;
  builder["token_type"] = "Bearer";
  builder["user"] = UserToJson(user);
  return builder.ExtractValue();
}

userver::formats::json::Value RoomToJson(const Room& room) {
  userver::formats::json::ValueBuilder builder;
  builder["room_number"] = room.room_number;
  builder["type"] = room.type;
  builder["price_per_night"] = room.price_per_night;
  builder["capacity"] = room.capacity;
  builder["is_available"] = room.is_available;
  if (!room.checkout_date.empty()) {
    builder["checkout_date"] = room.checkout_date;
  }
  return builder.ExtractValue();
}

userver::formats::json::Value HotelToJson(const Hotel& hotel) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = hotel.id;
  builder["name"] = hotel.name;
  builder["city"] = hotel.city;
  builder["address"] = hotel.address;
  builder["description"] = hotel.description;
  builder["stars"] = hotel.stars;
  
  if (!hotel.rooms.empty()) {
    userver::formats::json::ValueBuilder rooms_builder(userver::formats::common::Type::kArray);
    for (const auto& room : hotel.rooms) {
      rooms_builder.PushBack(RoomToJson(room));
    }
    builder["rooms"] = rooms_builder.ExtractValue();
  }
  
  return builder.ExtractValue();
}

userver::formats::json::Value BookingToJson(const Booking& booking) {
  userver::formats::json::ValueBuilder builder;
  builder["id"] = booking.id;
  builder["user_id"] = booking.user_id;
  builder["hotel_id"] = booking.hotel_id;
  builder["room_number"] = booking.room_number;
  builder["check_in"] = booking.check_in;
  builder["check_out"] = booking.check_out;
  builder["status"] = ToString(booking.status);
  builder["total_price"] = booking.total_price;
  return builder.ExtractValue();
}

userver::formats::json::Value UsersToJson(const std::vector<User>& users) {
  userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kArray);
  for (const auto& user : users) {
    builder.PushBack(UserToJson(user));
  }
  return builder.ExtractValue();
}

userver::formats::json::Value HotelsToJson(const std::vector<Hotel>& hotels) {
  userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kArray);
  for (const auto& hotel : hotels) {
    builder.PushBack(HotelToJson(hotel));
  }
  return builder.ExtractValue();
}

userver::formats::json::Value BookingsToJson(const std::vector<Booking>& bookings) {
  userver::formats::json::ValueBuilder builder(userver::formats::common::Type::kArray);
  for (const auto& booking : bookings) {
    builder.PushBack(BookingToJson(booking));
  }
  return builder.ExtractValue();
}

void StoreAuthContext(
    userver::server::request::RequestContext& context,
    const AuthContext& auth_context) {
  context.SetData<AuthContext>(std::string(kAuthContextKey), auth_context);
}

const AuthContext& RequireAuthContext(const userver::server::request::RequestContext& context) {
  const auto* auth_context = context.GetDataOptional<AuthContext>(std::string(kAuthContextKey));
  if (auth_context == nullptr) {
    throw std::runtime_error("Authentication context is missing");
  }
  return *auth_context;
}

}
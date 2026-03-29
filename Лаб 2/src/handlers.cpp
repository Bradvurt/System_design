#include "handlers.hpp"
#include "models.hpp"
#include "auth.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <userver/formats/json.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>

namespace {

using Json = userver::formats::json::Value;

std::string EscapeJson(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char ch : s) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    const char hex[] = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(static_cast<unsigned char>(ch) >> 4) & 0xF];
                    out += hex[static_cast<unsigned char>(ch) & 0xF];
                } else {
                    out += ch;
                }
        }
    }
    return out;
}

std::string JsonString(std::string_view s) {
    return std::string("\"") + EscapeJson(s) + "\"";
}

std::string UserToJson(const User& user) {
    return std::string("{") +
           "\"id\":" + std::to_string(user.id) + "," +
           "\"login\":" + JsonString(user.login) + "," +
           "\"first_name\":" + JsonString(user.first_name) + "," +
           "\"last_name\":" + JsonString(user.last_name) +
           "}";
}

std::string HotelToJson(const Hotel& hotel) {
    return std::string("{") +
           "\"id\":" + std::to_string(hotel.id) + "," +
           "\"name\":" + JsonString(hotel.name) + "," +
           "\"city\":" + JsonString(hotel.city) + "," +
           "\"address\":" + JsonString(hotel.address) +
           "}";
}

std::string BookingToJson(const Booking& booking) {
    return std::string("{") +
           "\"id\":" + std::to_string(booking.id) + "," +
           "\"user_id\":" + std::to_string(booking.user_id) + "," +
           "\"hotel_id\":" + std::to_string(booking.hotel_id) + "," +
           "\"check_in\":" + JsonString(booking.check_in) + "," +
           "\"check_out\":" + JsonString(booking.check_out) + "," +
           "\"active\":" + (booking.active ? std::string("true") : std::string("false")) +
           "}";
}

template <typename T, typename Fn>
std::string MakeArrayJson(const std::vector<T>& items, Fn&& to_json) {
    std::string out = "[";
    bool first = true;
    for (const auto& item : items) {
        if (!first) out += ',';
        first = false;
        out += to_json(item);
    }
    out += ']';
    return out;
}

std::string ErrorJson(std::string_view message) {
    return std::string("{\"error\":") + JsonString(message) + "}";
}

Json ParseBody(const userver::server::http::HttpRequest& request) {
    return userver::formats::json::FromString(request.RequestBody());
}

int ReadInt(const Json& json, std::string_view key) {
    return json[std::string(key)].As<int>();
}

std::string ReadString(const Json& json, std::string_view key) {
    return json[std::string(key)].As<std::string>();
}

int CurrentUserId(const userver::server::request::RequestContext& ctx) {
    return ctx.GetData<int>("user_id");
}

void SetJsonResponse(const userver::server::http::HttpRequest& request, userver::server::http::HttpStatus status) {
    request.SetResponseStatus(status);
    request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
}

} // namespace

namespace handlers {

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    try {
        const auto body = ParseBody(request);
        const auto login = ReadString(body, "login");
        const auto password = ReadString(body, "password");
        const auto first_name = ReadString(body, "first_name");
        const auto last_name = ReadString(body, "last_name");

        auto& storage = Storage::instance();
        const auto user = storage.addUser(login, auth::HashPassword(password), first_name, last_name);
        if (!user) {
            SetJsonResponse(request, userver::server::http::HttpStatus::kConflict);
            return ErrorJson("user already exists");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return UserToJson(*user);
    } catch (const std::exception& ex) {
        SetJsonResponse(request, userver::server::http::HttpStatus::kBadRequest);
        return ErrorJson(ex.what());
    }
}

std::string GetUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto& storage = Storage::instance();
    const auto login = request.GetArg("login");
    const auto first_mask = request.GetArg("first_name_mask");
    const auto last_mask = request.GetArg("last_name_mask");

    if (!login.empty()) {
        const auto user = storage.findUserByLogin(login);
        if (!user) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("user not found");
        }
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return UserToJson(*user);
    }

    const auto users = storage.searchUsers(login, first_mask, last_mask);
    request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
    return MakeArrayJson(users, UserToJson);
}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    try {
        const auto body = ParseBody(request);
        const auto login = ReadString(body, "login");
        const auto password = ReadString(body, "password");

        auto& storage = Storage::instance();
        const auto user = storage.findUserByLogin(login);
        if (!user || !auth::VerifyPassword(password, user->password_hash)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("invalid login or password");
        }

        const auto token = auth::GenerateToken(user->id);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return std::string("{\"token\":") + JsonString(token) + "}";
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return ErrorJson(ex.what());
    }
}

std::string CreateHotelHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    try {
        const auto body = ParseBody(request);
        const auto name = ReadString(body, "name");
        const auto city = ReadString(body, "city");
        const auto address = ReadString(body, "address");

        auto& storage = Storage::instance();
        const auto hotel = storage.addHotel(name, city, address);
        if (!hotel) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("hotel already exists");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return HotelToJson(*hotel);
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return ErrorJson(ex.what());
    }
}

std::string ListHotelsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto& storage = Storage::instance();
    const auto city = request.GetArg("city");
    const auto hotels = storage.listHotels(city);
    request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
    return MakeArrayJson(hotels, HotelToJson);
}

std::string CreateBookingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    try {
        const auto body = ParseBody(request);
        const auto hotel_id = ReadInt(body, "hotel_id");
        const auto check_in = ReadString(body, "check_in");
        const auto check_out = ReadString(body, "check_out");
        const auto user_id = CurrentUserId(context);

        if (check_in >= check_out) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("check_in must be earlier than check_out");
        }

        auto& storage = Storage::instance();
        const auto booking = storage.addBooking(user_id, hotel_id, check_in, check_out);
        if (!booking) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("user or hotel not found");
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return BookingToJson(*booking);
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return ErrorJson(ex.what());
    }
}

std::string ListUserBookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& storage = Storage::instance();
    const auto user_id = CurrentUserId(context);
    const auto bookings = storage.listBookingsByUser(user_id);
    request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
    return MakeArrayJson(bookings, BookingToJson);
}

std::string CancelBookingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    auto& storage = Storage::instance();
    const auto user_id = CurrentUserId(context);

    try {
        const auto booking_id = std::stoi(request.GetPathArg("id"));
        const auto booking = storage.findBookingById(booking_id);
        if (!booking) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("booking not found");
        }

        if (booking->user_id != user_id) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("booking belongs to another user");
        }

        if (!storage.cancelBooking(booking_id, user_id)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
            return ErrorJson("booking not found");
        }

        const auto cancelled = storage.findBookingById(booking_id);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return cancelled ? BookingToJson(*cancelled) : "{}";
    } catch (const std::exception&) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        request.GetHttpResponse().SetHeader(std::string("Content-Type"), std::string("application/json"));
        return ErrorJson("invalid booking id");
    }
}

} // namespace handlers

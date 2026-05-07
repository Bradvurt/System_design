#include "handlers.hpp"

#include <userver/formats/json.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

#include "models.hpp"

namespace {

using Json = userver::formats::json::Value;

// ---- JSON serialisation helpers (unchanged) ----
std::string EscapeJson(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char ch : s) {
        switch (ch) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
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
           "\"first_name\":" + JsonString(user.name) + "," +
           "\"last_name\":" + JsonString(user.surname) + "}";
}

std::string SessionToJson(const Session& session, const User& user) {
    return std::string("{") +
           "\"token\":" + JsonString(session.token) + "," +
           "\"user\":" + UserToJson(user) + "}";
}

std::string HotelToJson(const Hotel& hotel) {
    return std::string("{") +
           "\"id\":" + std::to_string(hotel.id) + "," +
           "\"name\":" + JsonString(hotel.name) + "," +
           "\"city\":" + JsonString(hotel.city) + "," +
           "\"address\":" + JsonString(hotel.address) + "}";
}

std::string BookingToJson(const Booking& booking) {
    return std::string("{") +
           "\"id\":" + std::to_string(booking.id) + "," +
           "\"user_id\":" + std::to_string(booking.user_id) + "," +
           "\"hotel_id\":" + std::to_string(booking.hotel_id) + "," +
           "\"check_in\":" + JsonString(booking.check_in) + "," +
           "\"check_out\":" + JsonString(booking.check_out) + "," +
           "\"active\":" + (booking.status == BookingStatus::kActive
                                ? std::string("true")
                                : std::string("false")) + "}";
}

template <typename Fn>
std::string MakeArrayJson(const std::vector<auto>& items, Fn&& to_json) {
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

std::int64_t ReadInt(const Json& json, std::string_view key) {
    return json[std::string(key)].As<std::int64_t>();
}

std::string ReadString(const Json& json, std::string_view key) {
    return json[std::string(key)].As<std::string>();
}

std::int64_t CurrentUserId(const userver::server::request::RequestContext& ctx) {
    return ctx.GetData<std::int64_t>("user_id");
}

void SetJsonResponse(const userver::server::http::HttpRequest& request,
                     const std::string& body,
                     int status = 200) {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kApplicationJson);
    request.SetResponseStatus(
        static_cast<userver::server::http::HttpStatus>(status));
    request.GetHttpResponse().SetData(body);   // <-- Исправлено
}

// Helper to create Storage from a component context
std::shared_ptr<Storage> MakeStorage(const userver::components::ComponentContext& context) {
    auto& pg = context.FindComponent<userver::components::Postgres>("postgres-db");
    return std::make_shared<Storage>(pg.GetCluster());
}

} // namespace

// ------------- Constructor implementations -------------
PingHandler::PingHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context) {}

CreateUserHandler::CreateUserHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

GetUserHandler::GetUserHandler(const userver::components::ComponentConfig& config,
                               const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

LoginHandler::LoginHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

CreateHotelHandler::CreateHotelHandler(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

ListHotelsHandler::ListHotelsHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

CreateBookingHandler::CreateBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

ListUserBookingsHandler::ListUserBookingsHandler(const userver::components::ComponentConfig& config,
                                                 const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

CancelBookingHandler::CancelBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context), storage_(MakeStorage(context)) {}

// ------------- Handler method implementations -------------
std::string PingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    SetJsonResponse(request, "{\"ok\":true}");
    return {};
}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        const auto json = ParseBody(request);
        RegisterUserRequest req;
        req.login    = ReadString(json, "login");
        req.name     = ReadString(json, "first_name");
        req.surname  = ReadString(json, "last_name");
        req.email    = ReadString(json, "email");
        req.password = ReadString(json, "password");

        const auto user = storage_->RegisterUser(req);
        LoginRequest login_req{req.login, req.password};
        const auto session = storage_->Login(login_req);
        SetJsonResponse(request, SessionToJson(session, user), 201);
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 400);
    }
    return {};
}

std::string GetUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        const auto login = request.GetArg("login");
        const auto first_name_mask = request.GetArg("first_name_mask");
        const auto last_name_mask = request.GetArg("last_name_mask");

        if (!login.empty() && first_name_mask.empty() && last_name_mask.empty()) {
            const auto user = storage_->FindUserByLogin(login);
            if (!user.has_value()) {
                SetJsonResponse(request, ErrorJson("User not found"), 404);
                return {};
            }
            SetJsonResponse(request, UserToJson(*user));
            return {};
        }

        const auto users = storage_->SearchUsers(first_name_mask, last_name_mask);
        SetJsonResponse(request, MakeArrayJson(users, UserToJson));
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 400);
    }
    return {};
}

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        const auto json = ParseBody(request);
        LoginRequest req;
        req.login    = ReadString(json, "login");
        req.password = ReadString(json, "password");

        const auto session = storage_->Login(req);
        const auto user = storage_->GetUserById(session.user_id);
        if (!user.has_value()) {
            SetJsonResponse(request, ErrorJson("User not found"), 500);
            return {};
        }
        SetJsonResponse(request, SessionToJson(session, *user));
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 401);
    }
    return {};
}

std::string CreateHotelHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    try {
        const auto user_id = CurrentUserId(context);
        const auto json = ParseBody(request);
        CreateHotelRequest req;
        req.name        = ReadString(json, "name");
        req.city        = ReadString(json, "city");
        req.address     = ReadString(json, "address");
        req.description = ReadString(json, "description");

        const auto hotel = storage_->CreateHotel(req, user_id);
        SetJsonResponse(request, HotelToJson(hotel), 201);
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 400);
    }
    return {};
}

std::string ListHotelsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        std::optional<std::string> city;
        const auto city_arg = request.GetArg("city");
        if (!city_arg.empty()) {
            city = city_arg;
        }
        const auto hotels = storage_->ListHotels(city);
        SetJsonResponse(request, MakeArrayJson(hotels, HotelToJson));
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 500);
    }
    return {};
}

std::string CreateBookingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    try {
        const auto user_id = CurrentUserId(context);
        const auto json = ParseBody(request);
        CreateBookingRequest req;
        req.hotel_id  = ReadInt(json, "hotel_id");
        req.check_in  = ReadString(json, "check_in");
        req.check_out = ReadString(json, "check_out");

        const auto booking = storage_->CreateBooking(req, user_id);
        SetJsonResponse(request, BookingToJson(booking), 201);
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 400);
    }
    return {};
}

std::string ListUserBookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    try {
        const auto user_id = CurrentUserId(context);
        const auto bookings = storage_->ListUserBookings(user_id);
        SetJsonResponse(request, MakeArrayJson(bookings, BookingToJson));
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 500);
    }
    return {};
}

std::string CancelBookingHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
    try {
        const auto user_id = CurrentUserId(context);
        const auto booking_id = std::stoll(request.GetPathArg("id"));
        storage_->CancelBooking(user_id, booking_id);
        request.SetResponseStatus(
            userver::server::http::HttpStatus::kNoContent);
    } catch (const std::exception& ex) {
        SetJsonResponse(request, ErrorJson(ex.what()), 400);
    }
    return {};
}
#include "handlers.hpp"

#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/redis/component.hpp>   // для userver::components::Redis

#include "auth.hpp"
#include "models.hpp"

namespace handlers {

using Json = userver::formats::json::Value;
using JsonBuilder = userver::formats::json::ValueBuilder;
using JsonType = userver::formats::json::Type;
using HttpStatus = userver::server::http::HttpStatus;

// --------------- Helpers ---------------

namespace {

std::string MakeJsonResponse(const userver::server::http::HttpRequest& request,
                              HttpStatus status, const std::string& body) {
    request.SetResponseStatus(status);
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kApplicationJson);
    return body;
}

std::string MakeErrorResponse(const userver::server::http::HttpRequest& request,
                               HttpStatus status, const std::string& code,
                               const std::string& message) {
    JsonBuilder builder;
    builder["error"]["code"] = code;
    builder["error"]["message"] = message;
    return MakeJsonResponse(request, status,
                            userver::formats::json::ToString(builder.ExtractValue()));
}

std::string MakeInternalError(const userver::server::http::HttpRequest& request) {
    return MakeErrorResponse(request, HttpStatus::kInternalServerError,
                              "internal_error", "Internal server error");
}

std::string UserToJson(const User& user) {
    JsonBuilder builder;
    builder["id"] = user.id;
    builder["login"] = user.login;
    builder["name"] = user.name;
    builder["surname"] = user.surname;
    builder["email"] = user.email;
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string HotelToJson(const Hotel& hotel) {
    JsonBuilder builder;
    builder["id"] = hotel.id;
    builder["name"] = hotel.name;
    builder["city"] = hotel.city;
    builder["address"] = hotel.address;
    builder["description"] = hotel.description;
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string BookingToJson(const Booking& booking) {
    JsonBuilder builder;
    builder["id"] = booking.id;
    builder["user_id"] = booking.user_id;
    builder["hotel_id"] = booking.hotel_id;
    builder["check_in"] = booking.check_in;
    builder["check_out"] = booking.check_out;
    builder["status"] = booking.active ? "active" : "cancelled";
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::string SessionToJson(const Session& session, const User& user) {
    JsonBuilder builder;
    builder["token"] = session.token;
    builder["user"] = JsonBuilder();
    builder["user"]["id"] = user.id;
    builder["user"]["login"] = user.login;
    builder["user"]["name"] = user.name;
    builder["user"]["surname"] = user.surname;
    builder["user"]["email"] = user.email;
    return userver::formats::json::ToString(builder.ExtractValue());
}

template <typename T, typename Fn>
std::string ArrayToJson(const std::vector<T>& items, Fn&& to_json) {
    JsonBuilder builder(JsonType::kArray);
    for (const auto& item : items) {
        builder.PushBack(
            userver::formats::json::FromString(to_json(item)));
    }
    return userver::formats::json::ToString(builder.ExtractValue());
}

Json ParseBody(const userver::server::http::HttpRequest& request) {
    return userver::formats::json::FromString(request.RequestBody());
}

std::string GetStringField(const Json& json, const std::string& name) {
    const auto field = json[name];
    if (field.IsMissing()) {
        throw std::runtime_error("Missing field: " + name);
    }
    return field.As<std::string>();
}

std::int64_t GetIntField(const Json& json, const std::string& name) {
    const auto field = json[name];
    if (field.IsMissing()) {
        throw std::runtime_error("Missing field: " + name);
    }
    return field.As<std::int64_t>();
}

void ValidateDate(const std::string& value, const std::string& field_name) {
    static const std::regex kDatePattern(R"(^\d{4}-\d{2}-\d{2}$)");
    if (!std::regex_match(value, kDatePattern)) {
        throw std::runtime_error("Field '" + field_name +
                                  "' must be in YYYY-MM-DD format");
    }
}

bool StartsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() &&
           value.substr(0, prefix.size()) == prefix;
}

std::int64_t ParseId(std::string_view value, std::string_view field_name) {
    try {
        return std::stoll(std::string(value));
    } catch (...) {
        throw std::runtime_error("Invalid " + std::string(field_name));
    }
}

}  // anonymous namespace

// --------------- Service Component ---------------

HotelBookingServiceComponent::HotelBookingServiceComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context) {
    // Правильный тип компонента Redis
    auto redis_client =
        context.FindComponent<userver::components::Redis>("redis-cache")
            .GetClient("cache-redis");
    cache_ = std::make_unique<CacheService>(redis_client);
    rate_limiter_ = std::make_unique<RateLimiter>(redis_client);
}

// --------------- Handler Base ---------------

HandlerBase::HandlerBase(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      service_(context.FindComponent<HotelBookingServiceComponent>()) {}

CacheService& HandlerBase::GetCache() const { return service_.GetCache(); }
RateLimiter& HandlerBase::GetRateLimiter() const { return service_.GetRateLimiter(); }

AuthContext HandlerBase::RequireAuth(
    const userver::server::http::HttpRequest& request) const {
    const auto& auth_header = request.GetHeader("Authorization");
    const std::string_view bearer_prefix = "Bearer ";
    if (!StartsWith(auth_header, bearer_prefix)) {
        throw std::runtime_error("unauthorized");
    }
    const std::string token(auth_header.substr(bearer_prefix.size()));
    auto session = Storage::Instance().FindSession(token);
    if (!session.has_value()) {
        throw std::runtime_error("unauthorized");
    }
    return AuthContext{session->user_id, session->login};
}

// --------------- Register Handler ---------------

std::string RegisterHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto json = ParseBody(request);
        auto login = Storage::Trim(GetStringField(json, "login"));
        auto name = Storage::Trim(GetStringField(json, "name"));
        auto surname = Storage::Trim(GetStringField(json, "surname"));
        auto email = Storage::Trim(GetStringField(json, "email"));
        auto password = GetStringField(json, "password");

        if (login.empty() || name.empty() || surname.empty() || email.empty() ||
            password.empty()) {
            return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                      "validation_error",
                                      "All fields are required");
        }

        auto password_hash = auth::HashPassword(password);
        auto user = Storage::Instance().AddUser(login, name, surname, email,
                                                 password_hash);
        if (!user.has_value()) {
            return MakeErrorResponse(request, HttpStatus::kConflict,
                                      "login_already_exists",
                                      "User with this login already exists");
        }

        // Auto-login after registration
        Session session;
        session.token = auth::GenerateToken(user->id, user->login);
        session.user_id = user->id;
        session.login = user->login;
        Storage::Instance().StoreSession(session);

        return MakeJsonResponse(request, HttpStatus::kCreated,
                                SessionToJson(session, *user));
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        if (msg == "unauthorized") {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "unauthorized", "Authentication required");
        }
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- Login Handler ---------------

std::string LoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto json = ParseBody(request);
        auto login = Storage::Normalize(GetStringField(json, "login"));
        auto password = GetStringField(json, "password");

        if (login.empty() || password.empty()) {
            return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                      "validation_error",
                                      "Login and password are required");
        }

        auto user = Storage::Instance().FindUserByLogin(login);
        if (!user.has_value() ||
            !auth::VerifyPassword(password, user->password_hash)) {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "invalid_credentials",
                                      "Invalid login or password");
        }

        Session session;
        session.token = auth::GenerateToken(user->id, user->login);
        session.user_id = user->id;
        session.login = user->login;
        Storage::Instance().StoreSession(session);

        return MakeJsonResponse(request, HttpStatus::kOk,
                                SessionToJson(session, *user));
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- User By Login Handler ---------------

std::string UserByLoginHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        const auto& login = request.GetPathArg("login");
        auto user = Storage::Instance().FindUserByLogin(std::string(login));
        if (!user.has_value()) {
            return MakeErrorResponse(request, HttpStatus::kNotFound,
                                      "user_not_found", "User was not found");
        }
        return MakeJsonResponse(request, HttpStatus::kOk, UserToJson(*user));
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- User Search Handler ---------------

std::string UserSearchHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto name = request.GetArg("name");
        auto surname = request.GetArg("surname");

        if (name.empty() && surname.empty()) {
            return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                      "validation_error",
                                      "At least one search parameter must be provided");
        }

        auto users = Storage::Instance().SearchUsers(name, surname);
        return MakeJsonResponse(
            request, HttpStatus::kOk,
            ArrayToJson(users, [](const User& u) { return UserToJson(u); }));
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- Hotels Handler ---------------

std::string HotelsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
            // --- GET: list hotels (with caching) ---
            std::optional<std::string> city_filter;
            auto city_arg = request.GetArg("city");
            if (!city_arg.empty()) {
                city_filter = city_arg;
            }

            // Try cache first
            auto cached = GetCache().GetHotels(city_filter);
            if (cached.has_value()) {
                return MakeJsonResponse(
                    request, HttpStatus::kOk,
                    ArrayToJson(*cached,
                                [](const Hotel& h) { return HotelToJson(h); }));
            }

            // Cache miss — load from storage
            auto hotels =
                Storage::Instance().ListHotels(city_filter.value_or(""));
            GetCache().SetHotels(city_filter, hotels, std::chrono::seconds{60});

            return MakeJsonResponse(
                request, HttpStatus::kOk,
                ArrayToJson(hotels,
                            [](const Hotel& h) { return HotelToJson(h); }));
        } else {
            // --- POST: create hotel ---
            auto auth = RequireAuth(request);
            auto json = ParseBody(request);
            auto name = Storage::Trim(GetStringField(json, "name"));
            auto city = Storage::Trim(GetStringField(json, "city"));
            auto address = Storage::Trim(GetStringField(json, "address"));
            auto description =
                Storage::Trim(json["description"].As<std::string>(""));

            if (name.empty() || city.empty() || address.empty()) {
                return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                          "validation_error",
                                          "name, city and address are required");
            }

            auto hotel = Storage::Instance().AddHotel(name, city, address,
                                                       description, auth.user_id);
            if (!hotel.has_value()) {
                return MakeInternalError(request);
            }

            // Invalidate hotels cache
            GetCache().InvalidateHotels();

            return MakeJsonResponse(request, HttpStatus::kCreated,
                                    HotelToJson(*hotel));
        }
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        if (msg == "unauthorized") {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "unauthorized", "Authentication required");
        }
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- Bookings Handler ---------------

std::string BookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto auth = RequireAuth(request);

        // --- Rate limiting check ---
        auto rate_decision = GetRateLimiter().CheckBookingCreation(auth.user_id);

        // Set rate limit headers (explicit std::string to resolve ambiguity)
        request.GetHttpResponse().SetHeader(std::string{"X-RateLimit-Limit"},
                                             std::to_string(rate_decision.limit));
        request.GetHttpResponse().SetHeader(std::string{"X-RateLimit-Remaining"},
                                             std::to_string(rate_decision.remaining));
        request.GetHttpResponse().SetHeader(std::string{"X-RateLimit-Reset"},
                                             std::to_string(rate_decision.reset_unix_seconds));

        if (!rate_decision.allowed) {
            return MakeErrorResponse(request, HttpStatus::kTooManyRequests,
                                      "rate_limit_exceeded",
                                      "Too many requests. Try again later.");
        }

        auto json = ParseBody(request);
        auto hotel_id = GetIntField(json, "hotel_id");
        auto check_in = GetStringField(json, "check_in");
        auto check_out = GetStringField(json, "check_out");

        ValidateDate(check_in, "check_in");
        ValidateDate(check_out, "check_out");

        if (check_out <= check_in) {
            return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                      "validation_error",
                                      "check_out must be after check_in");
        }

        auto booking = Storage::Instance().AddBooking(auth.user_id, hotel_id,
                                                       check_in, check_out);
        if (!booking.has_value()) {
            return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                      "booking_failed",
                                      "Could not create booking. Hotel may not exist "
                                      "or dates may overlap.");
        }

        // Invalidate user bookings cache
        GetCache().InvalidateUserBookings(auth.user_id);

        return MakeJsonResponse(request, HttpStatus::kCreated,
                                BookingToJson(*booking));
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        if (msg == "unauthorized") {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "unauthorized", "Authentication required");
        }
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- User Bookings Handler ---------------

std::string UserBookingsHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto auth = RequireAuth(request);
        auto user_id = ParseId(request.GetPathArg("user_id"), "user_id");

        // Only allow viewing own bookings
        if (auth.user_id != user_id) {
            return MakeErrorResponse(request, HttpStatus::kForbidden,
                                      "forbidden",
                                      "You can only view your own bookings");
        }

        // Try cache first
        auto cached = GetCache().GetUserBookings(user_id);
        if (cached.has_value()) {
            return MakeJsonResponse(
                request, HttpStatus::kOk,
                ArrayToJson(*cached,
                            [](const Booking& b) { return BookingToJson(b); }));
        }

        // Cache miss — load from storage
        auto bookings = Storage::Instance().ListUserBookings(user_id);
        GetCache().SetUserBookings(user_id, bookings, std::chrono::seconds{30});

        return MakeJsonResponse(
            request, HttpStatus::kOk,
            ArrayToJson(bookings,
                        [](const Booking& b) { return BookingToJson(b); }));
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        if (msg == "unauthorized") {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "unauthorized", "Authentication required");
        }
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

// --------------- Booking By ID Handler ---------------

std::string BookingByIdHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*context*/) const {
    try {
        auto auth = RequireAuth(request);
        auto booking_id = ParseId(request.GetPathArg("booking_id"), "booking_id");

        auto booking = Storage::Instance().FindBookingById(booking_id);
        if (!booking.has_value()) {
            return MakeErrorResponse(request, HttpStatus::kNotFound,
                                      "booking_not_found",
                                      "Booking was not found");
        }

        // Only the booking owner can cancel
        if (booking->user_id != auth.user_id) {
            return MakeErrorResponse(request, HttpStatus::kForbidden,
                                      "forbidden",
                                      "You can only cancel your own bookings");
        }

        bool cancelled = Storage::Instance().CancelBooking(booking_id);
        if (!cancelled) {
            return MakeErrorResponse(request, HttpStatus::kNotFound,
                                      "booking_not_found",
                                      "Booking was not found or already cancelled");
        }

        // Invalidate user bookings cache
        GetCache().InvalidateUserBookings(auth.user_id);

        request.SetResponseStatus(HttpStatus::kNoContent);
        return {};
    } catch (const std::runtime_error& ex) {
        std::string msg = ex.what();
        if (msg == "unauthorized") {
            return MakeErrorResponse(request, HttpStatus::kUnauthorized,
                                      "unauthorized", "Authentication required");
        }
        return MakeErrorResponse(request, HttpStatus::kBadRequest,
                                  "validation_error", msg);
    } catch (...) {
        return MakeInternalError(request);
    }
}

}  // namespace handlers
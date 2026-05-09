#include "cache.hpp"

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/redis/command_control.hpp>

namespace {

using Json = userver::formats::json::Value;
using JsonBuilder = userver::formats::json::ValueBuilder;
using JsonType = userver::formats::json::Type;               // <-- фикс

std::string SerializeHotels(const std::vector<Hotel>& hotels) {
    JsonBuilder builder;
    builder["type"] = "hotels";
    JsonBuilder items(JsonType::kArray);                      // <-- фикс
    for (const auto& h : hotels) {
        JsonBuilder item;
        item["id"] = h.id;
        item["name"] = h.name;
        item["city"] = h.city;
        item["address"] = h.address;
        item["description"] = h.description;
        item["created_by_user_id"] = h.created_by_user_id;
        items.PushBack(std::move(item));
    }
    builder["data"] = std::move(items);
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::vector<Hotel> DeserializeHotels(const std::string& json_str) {
    auto json = userver::formats::json::FromString(json_str);
    std::vector<Hotel> hotels;
    for (const auto& item : json["data"]) {
        Hotel h;
        h.id = item["id"].As<std::int64_t>();
        h.name = item["name"].As<std::string>();
        h.city = item["city"].As<std::string>();
        h.address = item["address"].As<std::string>();
        h.description = item["description"].As<std::string>();
        h.created_by_user_id = item["created_by_user_id"].As<std::int64_t>();
        hotels.push_back(std::move(h));
    }
    return hotels;
}

std::string SerializeBookings(const std::vector<Booking>& bookings) {
    JsonBuilder builder;
    builder["type"] = "bookings";
    JsonBuilder items(JsonType::kArray);                      // <-- фикс
    for (const auto& b : bookings) {
        JsonBuilder item;
        item["id"] = b.id;
        item["user_id"] = b.user_id;
        item["hotel_id"] = b.hotel_id;
        item["check_in"] = b.check_in;
        item["check_out"] = b.check_out;
        item["active"] = b.active;
        items.PushBack(std::move(item));
    }
    builder["data"] = std::move(items);
    return userver::formats::json::ToString(builder.ExtractValue());
}

std::vector<Booking> DeserializeBookings(const std::string& json_str) {
    auto json = userver::formats::json::FromString(json_str);
    std::vector<Booking> bookings;
    for (const auto& item : json["data"]) {
        Booking b;
        b.id = item["id"].As<std::int64_t>();
        b.user_id = item["user_id"].As<std::int64_t>();
        b.hotel_id = item["hotel_id"].As<std::int64_t>();
        b.check_in = item["check_in"].As<std::string>();
        b.check_out = item["check_out"].As<std::string>();
        b.active = item["active"].As<bool>();
        bookings.push_back(std::move(b));
    }
    return bookings;
}

}  // namespace

CacheService::CacheService(userver::storages::redis::ClientPtr redis_client)
    : redis_client_(std::move(redis_client)) {}

std::string CacheService::BuildHotelsCacheKey(
    const std::optional<std::string>& city_filter) {
    if (city_filter.has_value() && !city_filter->empty()) {
        return "cache:hotels:city:" + Storage::Normalize(*city_filter);
    }
    return "cache:hotels:all";
}

std::string CacheService::BuildUserBookingsCacheKey(std::int64_t user_id) {
    return "cache:bookings:" + std::to_string(user_id);
}

std::optional<std::vector<Hotel>> CacheService::GetHotels(
    const std::optional<std::string>& city_filter) {
    const auto key = BuildHotelsCacheKey(city_filter);
    try {
        // Добавлен CommandControl по умолчанию
        auto result = redis_client_->Get(key, {}).Get();
        if (!result) return std::nullopt;
        return DeserializeHotels(*result);
    } catch (...) {
        return std::nullopt;
    }
}

void CacheService::SetHotels(const std::optional<std::string>& city_filter,
                              const std::vector<Hotel>& hotels,
                              std::chrono::seconds ttl) {
    const auto key = BuildHotelsCacheKey(city_filter);
    try {
        // Set с TTL (миллисекунды) и CommandControl
        redis_client_->Set(
            key,
            SerializeHotels(hotels),
            std::chrono::duration_cast<std::chrono::milliseconds>(ttl),
            {}).Get();
    } catch (...) {
        // Ошибка кеширования не критична
    }
}

void CacheService::InvalidateHotels() {
    try {
        // Keys с дополнительными аргументами (max_keys, CommandControl)
        auto keys = redis_client_->Keys("cache:hotels:*", 1000, {}).Get();
        for (const auto& key : keys) {
            redis_client_->Del(key, {}).Get();      // Del с CommandControl
        }
    } catch (...) {
        // Ошибка инвалидации не критична
    }
}

std::optional<std::vector<Booking>> CacheService::GetUserBookings(
    std::int64_t user_id) {
    const auto key = BuildUserBookingsCacheKey(user_id);
    try {
        auto result = redis_client_->Get(key, {}).Get();   // <-- фикс
        if (!result) return std::nullopt;
        return DeserializeBookings(*result);
    } catch (...) {
        return std::nullopt;
    }
}

void CacheService::SetUserBookings(std::int64_t user_id,
                                    const std::vector<Booking>& bookings,
                                    std::chrono::seconds ttl) {
    const auto key = BuildUserBookingsCacheKey(user_id);
    try {
        redis_client_->Set(
            key,
            SerializeBookings(bookings),
            std::chrono::duration_cast<std::chrono::milliseconds>(ttl),
            {}).Get();
    } catch (...) {
        // ignore
    }
}

void CacheService::InvalidateUserBookings(std::int64_t user_id) {
    try {
        const auto key = BuildUserBookingsCacheKey(user_id);
        redis_client_->Del(key, {}).Get();                 // <-- фикс
    } catch (...) {
        // ignore
    }
}
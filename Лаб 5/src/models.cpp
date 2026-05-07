#include "models.hpp"
#include <algorithm>
#include <stdexcept>

Storage& Storage::instance() {
    static Storage instance;
    return instance;
}

void Storage::clearForTests() {
    std::lock_guard lock(mutex_);
    users_.clear();
    hotels_.clear();
    bookings_.clear();
    next_user_id_ = 0;
    next_hotel_id_ = 0;
    next_booking_id_ = 0;
    hotels_cache_.clear();
    user_bookings_cache_.clear();
    rate_limit_counters_.clear();
}

std::string Storage::ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<T>(std::tolower(c)); });
    return s;
}

bool Storage::MaskMatches(const std::string& value, const std::string& mask) {
    if (mask.empty()) return true;
    return ToLower(value).find(ToLower(mask)) != std::string::npos;
}

// --- User operations ---
std::optional<User> Storage::addUser(const std::string& login,
                                     const std::string& password_hash,
                                     const std::string& first_name,
                                     const std::string& last_name) {
    std::lock_guard lock(mutex_);
    if (std::any_of(users_.begin(), users_.end(),
                    [&](const User& u) { return u.login == login; })) {
        return std::nullopt;
    }
    users_.push_back(User{++next_user_id_, login, password_hash, first_name, last_name});
    return users_.back();
}

std::optional<User> Storage::findUserByLogin(const std::string& login) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(users_.begin(), users_.end(),
                           [&](const User& u) { return u.login == login; });
    if (it == users_.end()) return std::nullopt;
    return *it;
}

std::vector<User> Storage::searchUsers(const std::string& login,
                                       const std::string& first_name_mask,
                                       const std::string& last_name_mask) const {
    std::lock_guard lock(mutex_);
    std::vector<User> result;
    for (const auto& user : users_) {
        if (!login.empty() && user.login != login) continue;
        if (!MaskMatches(user.first_name, first_name_mask)) continue;
        if (!MaskMatches(user.last_name, last_name_mask)) continue;
        result.push_back(user);
    }
    return result;
}

// --- Hotel operations ---
std::optional<Hotel> Storage::addHotel(const std::string& name,
                                       const std::string& city,
                                       const std::string& address) {
    std::lock_guard lock(mutex_);
    hotels_.push_back(Hotel{++next_hotel_id_, name, city, address});
    invalidateHotelsCache();
    return hotels_.back();
}

std::vector<Hotel> Storage::listHotels(const std::string& city) const {
    using Clock = std::chrono::steady_clock;
    const auto cache_key = buildHotelsCacheKey(city.empty() ? std::optional<std::string>{} : city);
    const auto now = Clock::now();

    {
        std::lock_guard lock(mutex_);
        auto cache_it = hotels_cache_.find(cache_key);
        if (cache_it != hotels_cache_.end() && cache_it->second.expire_at > now) {
            return cache_it->second.hotels;
        }
    }

    std::lock_guard lock(mutex_);
    std::vector<Hotel> result;
    if (city.empty()) {
        result = hotels_;
    } else {
        for (const auto& hotel : hotels_) {
            if (ToLower(hotel.city) == ToLower(city)) {
                result.push_back(hotel);
            }
        }
    }
    hotels_cache_[cache_key] = CachedHotelsEntry{result, now + std::chrono::seconds(60)};
    return result;
}

std::optional<Hotel> Storage::findHotelById(int id) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(hotels_.begin(), hotels_.end(),
                           [&](const Hotel& h) { return h.id == id; });
    if (it == hotels_.end()) return std::nullopt;
    return *it;
}

// --- Booking operations ---
std::optional<Booking> Storage::addBooking(int user_id, int hotel_id,
                                          const std::string& check_in,
                                          const std::string& check_out) {
    std::lock_guard lock(mutex_);
    const auto hotel_it = std::find_if(hotels_.begin(), hotels_.end(),
                                       [&](const Hotel& h) { return h.id == hotel_id; });
    const auto user_it = std::find_if(users_.begin(), users_.end(),
                                      [&](const User& u) { return u.id == user_id; });
    if (hotel_it == hotels_.end() || user_it == users_.end()) {
        return std::nullopt;
    }
    if (check_in >= check_out) {
        return std::nullopt;
    }
    bookings_.push_back(Booking{++next_booking_id_, user_id, hotel_id, check_in, check_out, true});
    invalidateUserBookingsCache(user_id);
    return bookings_.back();
}

std::vector<Booking> Storage::listBookingsByUser(int user_id) const {
    using Clock = std::chrono::steady_clock;
    const auto now = Clock::now();

    {
        std::lock_guard lock(mutex_);
        auto cache_it = user_bookings_cache_.find(user_id);
        if (cache_it != user_bookings_cache_.end() && cache_it->second.expire_at > now) {
            return cache_it->second.bookings;
        }
    }

    std::lock_guard lock(mutex_);
    std::vector<Booking> result;
    for (const auto& booking : bookings_) {
        if (booking.user_id == user_id) {
            result.push_back(booking);
        }
    }
    user_bookings_cache_[user_id] = CachedUserBookingsEntry{result, now + std::chrono::seconds(30)};
    return result;
}

std::optional<Booking> Storage::findBookingById(int id) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(bookings_.begin(), bookings_.end(),
                           [&](const Booking& b) { return b.id == id; });
    if (it == bookings_.end()) return std::nullopt;
    return *it;
}

bool Storage::cancelBooking(int booking_id, int user_id) {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(bookings_.begin(), bookings_.end(),
                           [&](Booking& b) { return b.id == booking_id; });
    if (it == bookings_.end() || it->user_id != user_id) {
        return false;
    }
    it->active = false;
    invalidateUserBookingsCache(user_id);
    return true;
}

// --- Caching helpers (retained from lab5) ---
void Storage::invalidateHotelsCache() {
    std::lock_guard lock(mutex_);
    hotels_cache_.clear();
}

void Storage::invalidateUserBookingsCache(int user_id) {
    std::lock_guard lock(mutex_);
    user_bookings_cache_.erase(user_id);
}

std::string Storage::buildHotelsCacheKey(const std::optional<std::string>& city_filter) {
    if (!city_filter.has_value()) {
        return "all";
    }
    const auto city_value = city_filter.value();
    return city_value.empty() ? "all" : "city:" + ToLower(city_value);
}

// --- Rate limiting (retained from lab5) ---
Storage::RateLimitDecision Storage::checkBookingRateLimit(int user_id) {
    constexpr std::uint32_t kRequestLimit = 20;
    constexpr std::chrono::seconds kWindow{60};
    const auto now = std::chrono::steady_clock::now();

    std::lock_guard lock(mutex_);
    auto& [count, window_start] = rate_limit_counters_[user_id];
    if (now - window_start >= kWindow) {
        // Reset the window
        count = 0;
        window_start = now;
    }
    if (count >= kRequestLimit) {
        const auto reset_time = std::chrono::duration_cast<std::chrono::seconds>(
            (window_start + kWindow - now)).count();
        return {false, kRequestLimit, 0, static_cast<std::int64_t>(reset_time)};
    }
    count++;
    const auto remaining = kRequestLimit - count;
    return {true, kRequestLimit, remaining, 0};
}
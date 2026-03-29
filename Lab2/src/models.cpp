#include "models.hpp"

#include <algorithm>
#include <cctype>

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
}

std::string Storage::ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool Storage::MaskMatches(const std::string& value, const std::string& mask) {
    if (mask.empty()) return true;
    return ToLower(value).find(ToLower(mask)) != std::string::npos;
}

std::optional<User> Storage::addUser(const std::string& login, const std::string& password_hash,
                                     const std::string& first_name, const std::string& last_name) {
    std::lock_guard lock(mutex_);
    if (std::any_of(users_.begin(), users_.end(), [&](const User& u) { return u.login == login; })) {
        return std::nullopt;
    }

    users_.push_back(User{++next_user_id_, login, password_hash, first_name, last_name});
    return users_.back();
}

std::optional<User> Storage::findUserByLogin(const std::string& login) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(users_.begin(), users_.end(), [&](const User& u) { return u.login == login; });
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

std::optional<Hotel> Storage::addHotel(const std::string& name, const std::string& city,
                                       const std::string& address) {
    std::lock_guard lock(mutex_);
    hotels_.push_back(Hotel{++next_hotel_id_, name, city, address});
    return hotels_.back();
}

std::vector<Hotel> Storage::listHotels(const std::string& city) const {
    std::lock_guard lock(mutex_);
    std::vector<Hotel> result;
    for (const auto& hotel : hotels_) {
        if (!city.empty() && ToLower(hotel.city) != ToLower(city)) continue;
        result.push_back(hotel);
    }
    return result;
}

std::optional<Hotel> Storage::findHotelById(int id) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(hotels_.begin(), hotels_.end(), [&](const Hotel& h) { return h.id == id; });
    if (it == hotels_.end()) return std::nullopt;
    return *it;
}

std::optional<Booking> Storage::addBooking(int user_id, int hotel_id,
                                           const std::string& check_in,
                                           const std::string& check_out) {
    std::lock_guard lock(mutex_);
    const auto hotel_it = std::find_if(hotels_.begin(), hotels_.end(), [&](const Hotel& h) { return h.id == hotel_id; });
    const auto user_it = std::find_if(users_.begin(), users_.end(), [&](const User& u) { return u.id == user_id; });
    if (hotel_it == hotels_.end() || user_it == users_.end()) {
        return std::nullopt;
    }
    if (check_in >= check_out) {
        return std::nullopt;
    }

    bookings_.push_back(Booking{++next_booking_id_, user_id, hotel_id, check_in, check_out, true});
    return bookings_.back();
}

std::vector<Booking> Storage::listBookingsByUser(int user_id) const {
    std::lock_guard lock(mutex_);
    std::vector<Booking> result;
    for (const auto& booking : bookings_) {
        if (booking.user_id == user_id) {
            result.push_back(booking);
        }
    }
    return result;
}

std::optional<Booking> Storage::findBookingById(int id) const {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(bookings_.begin(), bookings_.end(), [&](const Booking& b) { return b.id == id; });
    if (it == bookings_.end()) return std::nullopt;
    return *it;
}

bool Storage::cancelBooking(int booking_id, int user_id) {
    std::lock_guard lock(mutex_);
    auto it = std::find_if(bookings_.begin(), bookings_.end(), [&](Booking& b) {
        return b.id == booking_id;
    });
    if (it == bookings_.end() || it->user_id != user_id) {
        return false;
    }
    it->active = false;
    return true;
}

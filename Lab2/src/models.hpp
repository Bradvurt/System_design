#pragma once

#include <optional>
#include <string>
#include <vector>
#include <mutex>

struct User {
    int id{};
    std::string login;
    std::string password_hash;
    std::string first_name;
    std::string last_name;
};

struct Hotel {
    int id{};
    std::string name;
    std::string city;
    std::string address;
};

struct Booking {
    int id{};
    int user_id{};
    int hotel_id{};
    std::string check_in;
    std::string check_out;
    bool active{true};
};

class Storage {
public:
    static Storage& instance();

    std::optional<User> addUser(const std::string& login, const std::string& password_hash,
                                const std::string& first_name, const std::string& last_name);
    std::optional<User> findUserByLogin(const std::string& login) const;
    std::vector<User> searchUsers(const std::string& login,
                                  const std::string& first_name_mask,
                                  const std::string& last_name_mask) const;

    std::optional<Hotel> addHotel(const std::string& name, const std::string& city,
                                  const std::string& address);
    std::vector<Hotel> listHotels(const std::string& city) const;
    std::optional<Hotel> findHotelById(int id) const;

    std::optional<Booking> addBooking(int user_id, int hotel_id,
                                      const std::string& check_in,
                                      const std::string& check_out);
    std::vector<Booking> listBookingsByUser(int user_id) const;
    std::optional<Booking> findBookingById(int id) const;
    bool cancelBooking(int booking_id, int user_id);

    void clearForTests();

private:
    static bool MaskMatches(const std::string& value, const std::string& mask);
    static std::string ToLower(std::string s);

    mutable std::mutex mutex_;
    std::vector<User> users_;
    std::vector<Hotel> hotels_;
    std::vector<Booking> bookings_;
    int next_user_id_{0};
    int next_hotel_id_{0};
    int next_booking_id_{0};
};

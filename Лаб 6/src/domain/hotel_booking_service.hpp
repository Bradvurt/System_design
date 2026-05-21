#pragma once
#include <userver/storages/mongo/pool.hpp>
#include <userver/urabbitmq/client.hpp>
#include "domain/models.hpp"
#include <memory>
#include <optional>
#include <mutex>
#include <unordered_map>

namespace booking {

class HotelBookingService final {
public:
    explicit HotelBookingService(userver::storages::mongo::PoolPtr mongo_pool,
                                 std::shared_ptr<userver::urabbitmq::Client> rmq_client);

    // Commands (Write operations)
    User RegisterUser(const RegisterUserRequest& request);
    Session Login(const LoginRequest& request);
    std::optional<Session> Authenticate(const std::string& token) const;
    Hotel CreateHotel(const CreateHotelRequest& request, std::int64_t created_by_user_id);
    Booking CreateBooking(const CreateBookingRequest& request, std::int64_t user_id);
    void CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id);

    // Queries (Read operations)
    std::optional<User> GetUserById(std::int64_t user_id) const;
    std::optional<User> FindUserByLogin(const std::string& login) const;
    std::vector<User> SearchUsers(const std::string& name_mask, const std::string& surname_mask) const;
    std::optional<Hotel> GetHotelById(std::int64_t hotel_id) const;
    std::vector<Hotel> ListHotels(const std::optional<std::string>& city) const;
    std::vector<Booking> ListUserBookings(std::int64_t requester_user_id, std::int64_t target_user_id) const;

private:
    // Helpers
    static std::string HashPassword(const std::string& value);
    static std::string GenerateToken(std::string_view login, std::int64_t user_id);
    std::int64_t NextId(const std::string& collection_name);

    // Data
    userver::storages::mongo::PoolPtr mongo_pool_;
    std::shared_ptr<userver::urabbitmq::Client> rmq_client_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Session> sessions_; // Token cache
};

} // namespace booking
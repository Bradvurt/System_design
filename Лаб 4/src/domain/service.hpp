#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/formats/bson/document.hpp>
#include <userver/storages/mongo/pool.hpp>

#include "domain/models.hpp"

namespace lab4 {

class HotelBookingService final {
 public:
  explicit HotelBookingService(userver::storages::mongo::PoolPtr mongo_pool);

  // Аутентификация
  User RegisterUser(const RegisterUserRequest& request);
  Session Login(const LoginRequest& request);
  std::optional<Session> Authenticate(const std::string& token) const;

  // Пользователи
  std::optional<User> GetUserById(std::int64_t user_id) const;
  std::optional<User> FindUserByLogin(const std::string& login) const;
  std::vector<User> SearchUsers(const std::string& name_mask, const std::string& surname_mask) const;

  // Отели
  Hotel CreateHotel(const CreateHotelRequest& request, std::int64_t created_by_user_id);
  std::optional<Hotel> GetHotelById(std::int64_t hotel_id) const;
  std::vector<Hotel> ListHotels(const std::optional<std::string>& city) const;

  // Бронирования
  Booking CreateBooking(const CreateBookingRequest& request, std::int64_t user_id);
  std::vector<Booking> ListUserBookings(std::int64_t requester_user_id, std::int64_t target_user_id) const;
  void CancelBooking(std::int64_t requester_user_id, std::int64_t booking_id);

 private:
  // Вспомогательные методы
  static std::string Normalize(const std::string& value);
  static std::string Trim(const std::string& value);
  static void ValidateDate(const std::string& value, const std::string& field_name);
  static bool DatesOverlap(const Booking& left, const Booking& right);
  static std::string HashPassword(const std::string& value);
  static std::string GenerateToken(std::string_view login, std::int64_t user_id);

  // Парсинг документов MongoDB
  static User ParseUser(const userver::formats::bson::Document& doc);
  static Hotel ParseHotel(const userver::formats::bson::Document& doc);
  static Booking ParseBooking(const userver::formats::bson::Document& doc);
  static Room ParseRoom(const userver::formats::bson::Document& doc);
  
  // Работа с ID и проверками
  std::int64_t NextId(const std::string& collection_name) const;
  User RequireUser(std::int64_t user_id) const;
  Hotel RequireHotel(std::int64_t hotel_id) const;

  // Данные
  mutable std::mutex mutex_;
  std::unordered_map<std::string, Session> sessions_;  // Кэш сессий
  userver::storages::mongo::PoolPtr mongo_pool_;
};

} // namespace lab4
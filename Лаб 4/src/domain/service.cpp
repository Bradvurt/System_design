#include "domain/service.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <utility>

#include <userver/formats/bson/inline.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/utils/datetime.hpp>

namespace lab4 {

namespace {

using userver::formats::bson::MakeDoc;
using userver::formats::bson::MakeArray;

const std::string kUsersCollection = "users";
const std::string kHotelsCollection = "hotels";
const std::string kBookingsCollection = "bookings";
const std::string kSessionsCollection = "sessions";
const std::string kIdSequenceCollection = "id_sequences";

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string Fnv1aHex(std::string_view value) {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;

  std::uint64_t hash = kOffsetBasis;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= kPrime;
  }

  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

userver::storages::mongo::Collection GetCollection(
    const userver::storages::mongo::PoolPtr& pool,
    const std::string& name) {
  return pool->GetCollection(name);
}

std::string BsonToString(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return "";
  }
  if (value.IsString()) {
    return value.As<std::string>();
  }
  return "";
}

std::int64_t BsonToInt64(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return 0;
  }
  if (value.IsInt64()) {
    return value.As<std::int64_t>();
  }
  if (value.IsInt32()) {
    return static_cast<std::int64_t>(value.As<int32_t>());
  }
  if (value.IsDouble()) {
    return static_cast<std::int64_t>(value.As<double>());
  }
  return 0;
}

double BsonToDouble(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return 0.0;
  }
  if (value.IsDouble()) {
    return value.As<double>();
  }
  if (value.IsInt32()) {
    return static_cast<double>(value.As<int32_t>());
  }
  if (value.IsInt64()) {
    return static_cast<double>(value.As<std::int64_t>());
  }
  if (value.IsDecimal128()) {
    // Преобразуем Decimal128 в строку, затем в double
    const auto& decimal = value.As<userver::formats::bson::Decimal128>();
    return std::stod(decimal.ToString());
  }
  return 0.0;
}

bool BsonToBool(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return false;
  }
  if (value.IsBool()) {
    return value.As<bool>();
  }
  return false;
}

std::string DateToString(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return "";
  }
  if (value.IsDateTime()) {
    auto tp = value.As<std::chrono::system_clock::time_point>();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
    gmtime_r(&t, &tm);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
  }
  return "";
}

std::string DateTimeToString(const userver::formats::bson::Value& value) {
  if (value.IsMissing() || value.IsNull()) {
    return "";
  }
  if (value.IsDateTime()) {
    auto tp = value.As<std::chrono::system_clock::time_point>();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
    gmtime_r(&t, &tm);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
  }
  return "";
}

std::chrono::system_clock::time_point StringToTimePoint(const std::string& date_str) {
  std::tm tm = {};
  std::istringstream ss(date_str);
  
  // Пробуем парсить с временем
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  if (!ss.fail()) {
    return std::chrono::system_clock::from_time_t(timegm(&tm));
  }
  
  // Пробуем парсить только дату
  ss.clear();
  ss.str(date_str);
  ss >> std::get_time(&tm, "%Y-%m-%d");
  if (!ss.fail()) {
    return std::chrono::system_clock::from_time_t(timegm(&tm));
  }
  
  return std::chrono::system_clock::now();
}

} // namespace

HotelBookingService::HotelBookingService(userver::storages::mongo::PoolPtr mongo_pool)
    : mongo_pool_(std::move(mongo_pool)) {}

std::string HotelBookingService::Trim(const std::string& value) {
  const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  }).base();
  if (begin >= end) {
    return {};
  }
  return std::string(begin, end);
}

std::string HotelBookingService::Normalize(const std::string& value) {
  return ToLower(Trim(value));
}

void HotelBookingService::ValidateDate(const std::string& value, const std::string& field_name) {
  static const std::regex kDatePattern(R"(^\d{4}-\d{2}-\d{2}$)");
  if (!std::regex_match(value, kDatePattern)) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date",
        "Field '" + field_name + "' must be in YYYY-MM-DD format");
  }
}

bool HotelBookingService::DatesOverlap(const Booking& left, const Booking& right) {
  return !(left.check_out <= right.check_in || left.check_in >= right.check_out);
}

std::string HotelBookingService::HashPassword(const std::string& value) {
  return Fnv1aHex("password:" + value);
}

std::string HotelBookingService::GenerateToken(
    std::string_view login,
    std::int64_t user_id) {
  const auto seed = std::string(login) + ":" + std::to_string(user_id) + ":" + Fnv1aHex(login);
  return "session-" + Fnv1aHex(seed);
}

User HotelBookingService::ParseUser(const userver::formats::bson::Document& doc) {
  User user;
  user.id = BsonToInt64(doc["id"]);
  user.login = BsonToString(doc["login"]);
  user.first_name = BsonToString(doc["first_name"]);
  user.last_name = BsonToString(doc["last_name"]);
  user.email = BsonToString(doc["email"]);
  user.phone = BsonToString(doc["phone"]);
  user.password_hash = BsonToString(doc["password_hash"]);
  
  user.created_at = DateTimeToString(doc["created_at"]);
  user.updated_at = DateTimeToString(doc["updated_at"]);
  
  return user;
}

Room HotelBookingService::ParseRoom(const userver::formats::bson::Document& doc) {
  Room room;
  room.room_number = BsonToString(doc["room_number"]);
  room.type = BsonToString(doc["type"]);
  room.price_per_night = BsonToDouble(doc["price_per_night"]);
  room.capacity = static_cast<int>(BsonToInt64(doc["capacity"]));
  room.is_available = BsonToBool(doc["is_available"]);
  
  if (doc.HasMember("checkout_date") && !doc["checkout_date"].IsNull()) {
    room.checkout_date = DateToString(doc["checkout_date"]);
  }
  
  return room;
}

Hotel HotelBookingService::ParseHotel(const userver::formats::bson::Document& doc) {
  Hotel hotel;
  hotel.id = BsonToInt64(doc["id"]);
  hotel.name = BsonToString(doc["name"]);
  hotel.description = BsonToString(doc["description"]);
  hotel.city = BsonToString(doc["city"]);
  hotel.address = BsonToString(doc["address"]);
  hotel.stars = static_cast<int>(BsonToInt64(doc["stars"]));
  hotel.created_by_user_id = BsonToInt64(doc["created_by_user_id"]);
  
  hotel.created_at = DateTimeToString(doc["created_at"]);
  hotel.updated_at = DateTimeToString(doc["updated_at"]);
  
  if (doc.HasMember("rooms") && doc["rooms"].IsArray()) {
    for (const auto& room_doc : doc["rooms"]) {
      hotel.rooms.push_back(ParseRoom(room_doc.As<userver::formats::bson::Document>()));
    }
  }
  
  return hotel;
}

Booking HotelBookingService::ParseBooking(const userver::formats::bson::Document& doc) {
  Booking booking;
  booking.id = BsonToInt64(doc["id"]);
  booking.user_id = BsonToInt64(doc["user_id"]);
  booking.hotel_id = BsonToInt64(doc["hotel_id"]);
  booking.room_number = BsonToString(doc["room_number"]);
  
  booking.check_in = DateToString(doc["check_in"]);
  booking.check_out = DateToString(doc["check_out"]);
  
  const auto status = BsonToString(doc["status"]);
  if (status == "cancelled") {
    booking.status = BookingStatus::kCancelled;
  } else if (status == "completed") {
    booking.status = BookingStatus::kCompleted;
  } else {
    booking.status = BookingStatus::kActive;
  }
  
  booking.total_price = BsonToDouble(doc["total_price"]);
  
  booking.created_at = DateTimeToString(doc["created_at"]);
  booking.updated_at = DateTimeToString(doc["updated_at"]);
  
  return booking;
}

std::int64_t HotelBookingService::NextId(const std::string& collection_name) const {
  auto id_coll = GetCollection(mongo_pool_, kIdSequenceCollection);
  
  auto filter = MakeDoc("_id", collection_name);
  auto update = MakeDoc("$inc", MakeDoc("next_id", static_cast<std::int64_t>(1)));
  
  // Используем UpdateOne без дополнительных опций (upsert не поддерживается в этом API)
  try {
    id_coll.UpdateOne(filter, update);
  } catch (const std::exception&) {
    // Если документ не существует, создаем его
    id_coll.InsertOne(MakeDoc("_id", collection_name, "next_id", static_cast<std::int64_t>(1)));
  }
  
  // Получаем текущее значение
  auto result = id_coll.FindOne(filter);
  if (result.has_value()) {
    auto& doc = *result;
    if (doc.HasMember("next_id")) {
      return BsonToInt64(doc["next_id"]);
    }
  }
  
  // Fallback: получаем максимальный ID из коллекции
  auto coll = GetCollection(mongo_pool_, collection_name);
  namespace options = userver::storages::mongo::options;
  
  auto cursor = coll.Find(
      MakeDoc(),
      options::Sort{{"id", options::Sort::kDescending}},
      options::Limit{1});
  
  for (const auto& doc : cursor) {
    return BsonToInt64(doc["id"]) + 1;
  }
  
  return 1;
}

User HotelBookingService::RequireUser(std::int64_t user_id) const {
  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  auto doc = users.FindOne(MakeDoc("id", user_id));
  if (!doc.has_value()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "user_not_found", "User was not found");
  }
  
  return ParseUser(*doc);
}

Hotel HotelBookingService::RequireHotel(std::int64_t hotel_id) const {
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  
  auto doc = hotels.FindOne(MakeDoc("id", hotel_id));
  if (!doc.has_value()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "hotel_not_found", "Hotel was not found");
  }
  
  return ParseHotel(*doc);
}

User HotelBookingService::RegisterUser(const RegisterUserRequest& request) {
  auto login = Normalize(request.login);
  auto first_name = Trim(request.name);
  auto last_name = Trim(request.surname);
  auto email = Trim(request.email);
  const auto& password = request.password;

  if (login.empty() || first_name.empty() || last_name.empty() || 
      email.empty() || password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login, name, surname, email and password are required");
  }

  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  auto existing = users.FindOne(MakeDoc("login", login));
  if (existing.has_value()) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "login_already_exists",
        "User with this login already exists");
  }
  
  existing = users.FindOne(MakeDoc("email", email));
  if (existing.has_value()) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "email_already_exists",
        "User with this email already exists");
  }

  User user;
  user.id = NextId(kUsersCollection);
  user.login = login;
  user.first_name = first_name;
  user.last_name = last_name;
  user.email = email;
  user.password_hash = HashPassword(password);
  
  auto now = std::chrono::system_clock::now();
  
  users.InsertOne(MakeDoc(
      "id", user.id,
      "login", user.login,
      "first_name", user.first_name,
      "last_name", user.last_name,
      "email", user.email,
      "password_hash", user.password_hash,
      "created_at", now,
      "updated_at", now
  ));

  return user;
}

Session HotelBookingService::Login(const LoginRequest& request) {
  const auto login = Normalize(request.login);
  if (login.empty() || request.password.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "login and password are required");
  }

  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  auto doc = users.FindOne(MakeDoc("login", login));
  if (!doc.has_value()) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  const auto user = ParseUser(*doc);
  if (user.password_hash != HashPassword(request.password)) {
    throw ServiceError(
        ServiceErrorKind::kUnauthorized,
        "invalid_credentials",
        "Invalid login or password");
  }

  std::lock_guard<std::mutex> lock(mutex_);
  
  Session session;
  session.user_id = user.id;
  session.login = user.login;
  session.token = GenerateToken(session.login, session.user_id);
  
  sessions_[session.token] = session;

  auto sessions_coll = GetCollection(mongo_pool_, kSessionsCollection);
  auto now = std::chrono::system_clock::now();
  
  // Используем replace с upsert через отдельные операции
  auto filter = MakeDoc("token", session.token);
  auto update = MakeDoc(
      "$set", MakeDoc(
          "user_id", session.user_id,
          "login", session.login,
          "created_at", now
      )
  );
  
  auto existing = sessions_coll.FindOne(filter);
  if (existing.has_value()) {
    sessions_coll.UpdateOne(filter, update);
  } else {
    sessions_coll.InsertOne(MakeDoc(
        "token", session.token,
        "user_id", session.user_id,
        "login", session.login,
        "created_at", now
    ));
  }
  
  return session;
}

std::optional<Session> HotelBookingService::Authenticate(const std::string& token) const {
  std::lock_guard<std::mutex> lock(mutex_);
  
  const auto it = sessions_.find(token);
  if (it != sessions_.end()) {
    return it->second;
  }
  
  auto sessions_coll = GetCollection(mongo_pool_, kSessionsCollection);
  auto doc = sessions_coll.FindOne(MakeDoc("token", token));
  
  if (!doc.has_value()) {
    return std::nullopt;
  }
  
  Session session;
  session.token = token;
  session.user_id = BsonToInt64((*doc)["user_id"]);
  session.login = BsonToString((*doc)["login"]);
  
  const_cast<HotelBookingService*>(this)->sessions_[token] = session;
  
  return session;
}

std::optional<User> HotelBookingService::GetUserById(std::int64_t user_id) const {
  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  auto doc = users.FindOne(MakeDoc("id", user_id));
  if (!doc.has_value()) {
    return std::nullopt;
  }
  
  return ParseUser(*doc);
}

std::optional<User> HotelBookingService::FindUserByLogin(const std::string& login) const {
  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  auto doc = users.FindOne(MakeDoc("login", Normalize(login)));
  if (!doc.has_value()) {
    return std::nullopt;
  }
  
  return ParseUser(*doc);
}

std::vector<User> HotelBookingService::SearchUsers(
    const std::string& name_mask,
    const std::string& surname_mask) const {
  
  const auto normalized_name = Trim(name_mask);
  const auto normalized_surname = Trim(surname_mask);
  
  if (normalized_name.empty() && normalized_surname.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "At least one search parameter must be provided");
  }

  auto users = GetCollection(mongo_pool_, kUsersCollection);
  
  userver::formats::bson::Value filter;
  
  if (!normalized_name.empty() && !normalized_surname.empty()) {
    filter = MakeDoc(
        "$and", MakeArray(
            MakeDoc("first_name", MakeDoc("$regex", normalized_name, "$options", "i")),
            MakeDoc("last_name", MakeDoc("$regex", normalized_surname, "$options", "i"))
        )
    );
  } else if (!normalized_name.empty()) {
    filter = MakeDoc("first_name", MakeDoc("$regex", normalized_name, "$options", "i"));
  } else {
    filter = MakeDoc("last_name", MakeDoc("$regex", normalized_surname, "$options", "i"));
  }

  namespace options = userver::storages::mongo::options;
  
  std::vector<User> result;
  for (const auto& doc : users.Find(
      filter, 
      options::Sort{{"id", options::Sort::kAscending}})) {
    result.push_back(ParseUser(doc));
  }
  
  return result;
}

Hotel HotelBookingService::CreateHotel(
    const CreateHotelRequest& request,
    std::int64_t created_by_user_id) {
  
  auto name = Trim(request.name);
  auto city = Trim(request.city);
  auto address = Trim(request.address);
  auto description = Trim(request.description);

  if (name.empty() || city.empty() || address.empty()) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "validation_error",
        "name, city and address are required");
  }

  (void)RequireUser(created_by_user_id);

  Hotel hotel;
  hotel.id = NextId(kHotelsCollection);
  hotel.name = name;
  hotel.city = city;
  hotel.address = address;
  hotel.description = description;
  hotel.created_by_user_id = created_by_user_id;
  hotel.rooms = request.rooms;
  
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  auto now = std::chrono::system_clock::now();
  
  // Строим массив номеров
  userver::formats::bson::ValueBuilder rooms_builder(userver::formats::common::Type::kArray);
  for (const auto& room : hotel.rooms) {
    userver::formats::bson::ValueBuilder room_builder;
    room_builder["room_number"] = room.room_number;
    room_builder["type"] = room.type;
    room_builder["price_per_night"] = room.price_per_night;
    room_builder["capacity"] = room.capacity;
    room_builder["is_available"] = room.is_available;
    
    if (!room.checkout_date.empty()) {
      room_builder["checkout_date"] = StringToTimePoint(room.checkout_date);
    } else {
      room_builder["checkout_date"] = userver::formats::bson::Value();
    }
    rooms_builder.PushBack(room_builder.ExtractValue());
  }
  
  hotels.InsertOne(MakeDoc(
      "id", hotel.id,
      "name", hotel.name,
      "description", hotel.description,
      "city", hotel.city,
      "address", hotel.address,
      "stars", 0,
      "created_by_user_id", hotel.created_by_user_id,
      "created_at", now,
      "updated_at", now,
      "rooms", rooms_builder.ExtractValue()
  ));

  return hotel;
}

std::optional<Hotel> HotelBookingService::GetHotelById(std::int64_t hotel_id) const {
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  
  auto doc = hotels.FindOne(MakeDoc("id", hotel_id));
  if (!doc.has_value()) {
    return std::nullopt;
  }
  
  return ParseHotel(*doc);
}

std::vector<Hotel> HotelBookingService::ListHotels(const std::optional<std::string>& city) const {
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  
  userver::formats::bson::Value filter = MakeDoc();
  if (city.has_value()) {
    const auto city_trimmed = Trim(*city);
    if (!city_trimmed.empty()) {
      filter = MakeDoc("city", MakeDoc("$regex", "^" + city_trimmed + "$", "$options", "i"));
    }
  }

  namespace options = userver::storages::mongo::options;
  
  std::vector<Hotel> result;
  for (const auto& doc : hotels.Find(
      filter, 
      options::Sort{{"id", options::Sort::kAscending}})) {
    result.push_back(ParseHotel(doc));
  }
  
  return result;
}

Booking HotelBookingService::CreateBooking(
    const CreateBookingRequest& request, 
    std::int64_t user_id) {
  
  ValidateDate(request.check_in, "check_in");
  ValidateDate(request.check_out, "check_out");
  
  if (request.check_in >= request.check_out) {
    throw ServiceError(
        ServiceErrorKind::kBadRequest,
        "invalid_date_range",
        "check_in must be earlier than check_out");
  }

  (void)RequireUser(user_id);
  auto hotel = RequireHotel(request.hotel_id);
  
  bool room_found = false;
  bool room_available = false;
  double room_price = 0.0;
  
  for (const auto& room : hotel.rooms) {
    if (room.room_number == request.room_number) {
      room_found = true;
      room_available = room.is_available;
      room_price = room.price_per_night;
      
      if (!room.checkout_date.empty() && room.checkout_date > request.check_in) {
        room_available = false;
      }
      break;
    }
  }
  
  if (!room_found) {
    throw ServiceError(
        ServiceErrorKind::kNotFound,
        "room_not_found",
        "Room '" + request.room_number + "' not found in this hotel");
  }
  
  if (!room_available) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "room_not_available",
        "Room '" + request.room_number + "' is not available");
  }

  Booking candidate;
  candidate.id = NextId(kBookingsCollection);
  candidate.user_id = user_id;
  candidate.hotel_id = request.hotel_id;
  candidate.room_number = request.room_number;
  candidate.check_in = request.check_in;
  candidate.check_out = request.check_out;
  candidate.status = BookingStatus::kActive;

  auto check_in_tp = StringToTimePoint(request.check_in + "T14:00:00Z");
  auto check_out_tp = StringToTimePoint(request.check_out + "T11:00:00Z");
  auto duration = std::chrono::duration_cast<std::chrono::hours>(check_out_tp - check_in_tp);
  int nights = duration.count() / 24;
  if (nights <= 0) nights = 1;
  
  candidate.total_price = room_price * nights;

  auto bookings = GetCollection(mongo_pool_, kBookingsCollection);
  
  auto conflict_filter = MakeDoc(
      "hotel_id", candidate.hotel_id,
      "room_number", candidate.room_number,
      "status", "active",
      "$or", MakeArray(
          MakeDoc(
              "check_in", MakeDoc("$lt", check_out_tp),
              "check_out", MakeDoc("$gt", check_in_tp)
          )
      )
  );
  
  auto conflict = bookings.FindOne(conflict_filter);
  
  if (conflict.has_value()) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_dates_conflict",
        "Room is already booked for the selected dates");
  }

  auto now = std::chrono::system_clock::now();
  
  bookings.InsertOne(MakeDoc(
      "id", candidate.id,
      "user_id", candidate.user_id,
      "hotel_id", candidate.hotel_id,
      "room_number", candidate.room_number,
      "check_in", check_in_tp,
      "check_out", check_out_tp,
      "status", "active",
      "total_price", candidate.total_price,
      "created_at", now,
      "updated_at", now
  ));
  
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  auto hotel_filter = MakeDoc("id", request.hotel_id, "rooms.room_number", request.room_number);
  auto hotel_update = MakeDoc(
      "$set", MakeDoc(
          "rooms.$.is_available", false,
          "rooms.$.checkout_date", check_out_tp,
          "updated_at", now
      )
  );
  
  hotels.UpdateOne(hotel_filter, hotel_update);

  return candidate;
}

std::vector<Booking> HotelBookingService::ListUserBookings(
    std::int64_t requester_user_id,
    std::int64_t target_user_id) const {
  
  (void)RequireUser(requester_user_id);
  (void)RequireUser(target_user_id);
  
  if (requester_user_id != target_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only access your own bookings");
  }

  auto bookings = GetCollection(mongo_pool_, kBookingsCollection);
  
  namespace options = userver::storages::mongo::options;
  
  std::vector<Booking> result;
  for (const auto& doc : bookings.Find(
      MakeDoc("user_id", target_user_id),
      options::Sort{{"id", options::Sort::kAscending}})) {
    result.push_back(ParseBooking(doc));
  }
  
  return result;
}

void HotelBookingService::CancelBooking(
    std::int64_t requester_user_id, 
    std::int64_t booking_id) {
  
  (void)RequireUser(requester_user_id);

  auto bookings = GetCollection(mongo_pool_, kBookingsCollection);
  
  auto booking_doc = bookings.FindOne(MakeDoc("id", booking_id));
  if (!booking_doc.has_value()) {
    throw ServiceError(ServiceErrorKind::kNotFound, "booking_not_found", "Booking was not found");
  }

  const auto booking = ParseBooking(*booking_doc);
  
  if (booking.user_id != requester_user_id) {
    throw ServiceError(
        ServiceErrorKind::kForbidden,
        "forbidden",
        "You can only cancel your own bookings");
  }
  
  if (booking.status == BookingStatus::kCancelled) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_already_cancelled",
        "Booking has already been cancelled");
  }
  
  if (booking.status == BookingStatus::kCompleted) {
    throw ServiceError(
        ServiceErrorKind::kConflict,
        "booking_already_completed",
        "Cannot cancel completed booking");
  }

  auto now = std::chrono::system_clock::now();
  
  auto booking_filter = MakeDoc("id", booking_id);
  auto booking_update = MakeDoc(
      "$set", MakeDoc(
          "status", "cancelled",
          "updated_at", now
      )
  );
  bookings.UpdateOne(booking_filter, booking_update);
  
  auto hotels = GetCollection(mongo_pool_, kHotelsCollection);
  auto hotel_filter = MakeDoc("id", booking.hotel_id, "rooms.room_number", booking.room_number);
  auto hotel_update = MakeDoc(
      "$set", MakeDoc(
          "rooms.$.is_available", true,
          "rooms.$.checkout_date", userver::formats::bson::Value(),
          "updated_at", now
      )
  );
  hotels.UpdateOne(hotel_filter, hotel_update);
}

} // namespace lab4
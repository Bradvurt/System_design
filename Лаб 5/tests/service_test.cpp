#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

#include "domain/service.hpp"

namespace {

void Check(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Fn>
void ExpectServiceError(Fn&& fn, lab2::ServiceErrorKind expected_kind, const std::string& expected_code) {
  try {
    fn();
    throw std::runtime_error("Expected ServiceError was not thrown");
  } catch (const lab2::ServiceError& ex) {
    Check(ex.GetKind() == expected_kind, "Unexpected error kind");
    Check(ex.GetCode() == expected_code, "Unexpected error code");
  }
}

void TestUserRegistrationAndLogin() {
  lab2::HotelBookingService service;

  const auto user = service.RegisterUser(
      {"traveler", "Ivan", "Petrov", "ivan@example.com", "secret123"});
  Check(user.id == 1, "first user id must be 1");
  Check(user.login == "traveler", "login normalized to lowercase");

  const auto session = service.Login({"traveler", "secret123"});
  Check(!session.token.empty(), "empty session token");
  Check(service.Authenticate(session.token).has_value(), "token missing in store");

  const auto found = service.FindUserByLogin("TRAVELER");
  Check(found.has_value(), "lookup TRAVELER vs traveler");
  Check(found->email == "ivan@example.com", "email mismatch after register");

  ExpectServiceError(
      [&] { service.RegisterUser({"traveler", "Ivan", "Petrov", "other@example.com", "secret123"}); },
      lab2::ServiceErrorKind::kConflict,
      "login_already_exists");
}

void TestHotelSearchAndBookingLifecycle() {
  lab2::HotelBookingService service;

  const auto first_user =
      service.RegisterUser({"alice", "Alice", "Brown", "alice@example.com", "password1"});
  const auto second_user =
      service.RegisterUser({"bob", "Bob", "Stone", "bob@example.com", "password2"});

  const auto hotel = service.CreateHotel(
      {"Nevsky View", "Saint Petersburg", "Nevsky 1", "Downtown hotel"},
      first_user.id);
  service.CreateHotel(
      {"Moscow Riverside", "Moscow", "Tverskaya 10", "Near the river"},
      first_user.id);

  const auto saint_petersburg_hotels = service.ListHotels(std::string("saint petersburg"));
  Check(saint_petersburg_hotels.size() == 1, "filter by city: expected 1 hotel");
  Check(saint_petersburg_hotels.front().id == hotel.id, "wrong hotel in city filter");

  const auto booking = service.CreateBooking({hotel.id, "2026-04-10", "2026-04-15"}, first_user.id);
  Check(booking.id == 1, "first booking id");
  Check(booking.status == lab2::BookingStatus::kActive, "status not active after create");

  const auto bookings = service.ListUserBookings(first_user.id, first_user.id);
  Check(bookings.size() == 1, "booking list length");

  ExpectServiceError(
      [&] { service.ListUserBookings(second_user.id, first_user.id); },
      lab2::ServiceErrorKind::kForbidden,
      "forbidden");

  ExpectServiceError(
      [&] { service.CreateBooking({hotel.id, "2026-04-12", "2026-04-18"}, second_user.id); },
      lab2::ServiceErrorKind::kConflict,
      "booking_dates_conflict");

  service.CancelBooking(first_user.id, booking.id);
  const auto updated_bookings = service.ListUserBookings(first_user.id, first_user.id);
  Check(
      updated_bookings.front().status == lab2::BookingStatus::kCancelled,
      "cancel did not stick");
}

void TestUserSearch() {
  lab2::HotelBookingService service;
  service.RegisterUser({"traveler-1", "Alexey", "Romanov", "one@example.com", "secret1"});
  service.RegisterUser({"traveler-2", "Alexandra", "Romanova", "two@example.com", "secret2"});
  service.RegisterUser({"traveler-3", "Maria", "Smirnova", "three@example.com", "secret3"});

  const auto users = service.SearchUsers("Alex", "roman");
  Check(users.size() == 2, "Alex* + *roman* => 2 rows");

  ExpectServiceError(
      [&] { service.SearchUsers("", ""); },
      lab2::ServiceErrorKind::kBadRequest,
      "validation_error");
}

void TestBookingCancelAndPermissions() {
  lab2::HotelBookingService service;

  const auto alice =
      service.RegisterUser({"alice2", "Alice", "Brown", "alice2@example.com", "password1"});
  const auto bob =
      service.RegisterUser({"bob2", "Bob", "Stone", "bob2@example.com", "password2"});

  const auto hotel =
      service.CreateHotel({"Central", "Kazan", "Bauman 1", ""}, alice.id);
  const auto booking =
      service.CreateBooking({hotel.id, "2026-07-01", "2026-07-10"}, alice.id);

  ExpectServiceError(
      [&] { service.CancelBooking(bob.id, booking.id); },
      lab2::ServiceErrorKind::kForbidden,
      "forbidden");

  ExpectServiceError(
      [&] { service.CancelBooking(alice.id, 9999); },
      lab2::ServiceErrorKind::kNotFound,
      "booking_not_found");

  service.CancelBooking(alice.id, booking.id);
  ExpectServiceError(
      [&] { service.CancelBooking(alice.id, booking.id); },
      lab2::ServiceErrorKind::kConflict,
      "booking_already_cancelled");
}

}

int main() {
  try {
    TestUserRegistrationAndLogin();
    TestHotelSearchAndBookingLifecycle();
    TestUserSearch();
    TestBookingCancelAndPermissions();
    std::cout << "All service tests passed\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}

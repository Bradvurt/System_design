#include "models.hpp"
#include "auth.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failed = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << " CHECK(" #cond ")\n"; \
        ++g_failed; \
    } \
} while (false)

#define REQUIRE(cond) do { \
    if (!(cond)) { \
        std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << " REQUIRE(" #cond ")\n"; \
        return false; \
    } \
} while (false)

bool TestStorageUsers() {
    auto& storage = Storage::instance();
    storage.clearForTests();

    auto user = storage.addUser("alice", "hash1", "Alice", "Ivanova");
    REQUIRE(user.has_value());
    CHECK(user->id == 1);
    CHECK(user->login == "alice");

    auto duplicate = storage.addUser("alice", "hash2", "A", "B");
    CHECK(!duplicate.has_value());

    auto found = storage.findUserByLogin("alice");
    REQUIRE(found.has_value());
    CHECK(found->first_name == "Alice");

    auto masked = storage.searchUsers("", "lic", "van");
    REQUIRE(masked.size() == 1);
    CHECK(masked.front().login == "alice");

    auto not_found = storage.findUserByLogin("missing");
    CHECK(!not_found.has_value());
    return true;
}

bool TestStorageHotelsAndBookings() {
    auto& storage = Storage::instance();
    storage.clearForTests();

    auto user = storage.addUser("alice", "hash1", "Alice", "Ivanova");
    auto hotel1 = storage.addHotel("Grand Hotel", "Moscow", "Tverskaya 1");
    auto hotel2 = storage.addHotel("River Hotel", "Berlin", "Main 2");
    REQUIRE(user.has_value());
    REQUIRE(hotel1.has_value());
    REQUIRE(hotel2.has_value());

    auto hotels_moscow = storage.listHotels("moscow");
    REQUIRE(hotels_moscow.size() == 1);
    CHECK(hotels_moscow.front().name == "Grand Hotel");

    auto bad_booking = storage.addBooking(user->id, hotel1->id, "2026-05-05", "2026-05-01");
    CHECK(!bad_booking.has_value());

    auto booking = storage.addBooking(user->id, hotel1->id, "2026-04-01", "2026-04-05");
    REQUIRE(booking.has_value());
    CHECK(booking->id == 1);
    CHECK(booking->active);

    auto bookings = storage.listBookingsByUser(user->id);
    REQUIRE(bookings.size() == 1);
    CHECK(bookings.front().hotel_id == hotel1->id);

    CHECK(storage.cancelBooking(booking->id, user->id));
    auto cancelled = storage.findBookingById(booking->id);
    REQUIRE(cancelled.has_value());
    CHECK(!cancelled->active);

    CHECK(!storage.cancelBooking(booking->id + 100, user->id));
    return true;
}

bool TestAuthRoundtrip() {
    const std::string secret = "unit-test-secret-key-1234567890";
    const auto token = auth::GenerateToken(42, secret);
    auto user_id = auth::ValidateToken(token, secret);
    REQUIRE(user_id.has_value());
    CHECK(*user_id == 42);

    CHECK(!auth::ValidateToken(token, "another-secret").has_value());
    CHECK(auth::VerifyPassword("pass", auth::HashPassword("pass")));
    CHECK(!auth::VerifyPassword("pass", auth::HashPassword("other")));
    return true;
}

} // namespace

int main() {
    const std::vector<bool (*)()> tests = {
        &TestStorageUsers,
        &TestStorageHotelsAndBookings,
        &TestAuthRoundtrip,
    };

    for (const auto test : tests) {
        if (!test()) {
            ++g_failed;
        }
    }

    if (g_failed != 0) {
        std::cerr << g_failed << " test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All unit tests passed\n";
    return EXIT_SUCCESS;
}

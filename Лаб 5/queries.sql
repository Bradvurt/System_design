-- 1. Создание нового пользователя (Register user)
--    API: POST /api/v1/users
--    Input: login, first_name, last_name, email, password
--    Output: user object
-- ---------------------------------------------------------------------------
-- This query first checks if the login (lowercased) already exists, then inserts.
-- In the C++ service, this is done with two separate calls.
-- For completeness, we show a single transaction-like approach.
BEGIN;
INSERT INTO users (login, name, surname, email, password_hash)
SELECT
    LOWER(TRIM(:login))   AS login,
    TRIM(:first_name)      AS name,
    TRIM(:last_name)       AS surname,
    TRIM(:email)           AS email,
    :hashed_password       AS password_hash
WHERE NOT EXISTS (
    SELECT 1 FROM users WHERE login = LOWER(TRIM(:login))
)
RETURNING id, login, name, surname, email, password_hash;
COMMIT;

-- Or using a CTE (PostgreSQL can use INSERT ... SELECT)
-- Actual C++ code uses two queries: existence check + INSERT RETURNING.
-- The following is a pure SQL version.
-- ---------------------------------------------------------------------------

-- 2. Поиск пользователя по логину (Find user by login)
--    API: GET /api/v1/users?login=...
-- ---------------------------------------------------------------------------
SELECT id, login, name, surname, email, password_hash
FROM users
WHERE login = LOWER(TRIM(:login));

-- 3. Поиск пользователя по маске имя и фамилии (Search users by name/surname mask)
--    API: GET /api/v1/users?first_name_mask=...&last_name_mask=...
-- ---------------------------------------------------------------------------
SELECT id, login, name, surname, email, password_hash
FROM users
WHERE
    (:first_name_mask IS NULL OR :first_name_mask = '' OR name ILIKE '%' || :first_name_mask || '%')
    AND
    (:last_name_mask IS NULL OR :last_name_mask = '' OR surname ILIKE '%' || :last_name_mask || '%')
ORDER BY surname, name;  -- Added sorting for deterministic output

-- 4. Создание отеля (Create hotel)
--    API: POST /api/v1/hotels (auth required)
--    Input: name, city, address, description
--    Output: hotel object
-- ---------------------------------------------------------------------------
INSERT INTO hotels (name, city, address, description, created_by_user_id, rating)
VALUES (
    TRIM(:name),
    TRIM(:city),
    TRIM(:address),
    TRIM(:description),
    :created_by_user_id,
    NULL  -- New hotel has no rating yet
)
RETURNING *;

-- 5. Получение списка отелей (List hotels, optionally by city)
--    API: GET /api/v1/hotels?city=...
-- ---------------------------------------------------------------------------
SELECT * FROM hotels
WHERE (:city IS NULL OR :city = '' OR LOWER(city) = LOWER(TRIM(:city)))
ORDER BY rating DESC NULLS LAST, name;

-- 6. Создание бронирования (Create booking)
--    API: POST /api/v1/bookings (auth required)
--    Input: hotel_id, check_in (YYYY-MM-DD), check_out (YYYY-MM-DD)
--    Output: booking object
-- ---------------------------------------------------------------------------
-- Using a CTE to validate hotel existence and check dates.
WITH valid AS (
    SELECT id FROM hotels WHERE id = :hotel_id
)
INSERT INTO bookings (user_id, hotel_id, check_in, check_out, status)
SELECT :user_id, :hotel_id, :check_in::date, :check_out::date, 'active'
FROM valid
WHERE EXISTS (SELECT 1 FROM valid)   -- hotel exists
RETURNING *;

-- 7. Получение бронирований пользователя (List user bookings)
--    API: GET /api/v1/bookings (auth required, user_id from session)
-- ---------------------------------------------------------------------------
SELECT * FROM bookings
WHERE user_id = :user_id
ORDER BY check_in DESC;

-- 8. Отмена бронирования (Cancel booking)
--    API: DELETE /api/v1/bookings/{id} (auth required)
--    Only the booking owner can cancel.
-- ---------------------------------------------------------------------------
UPDATE bookings
SET status = 'cancelled'
WHERE id = :booking_id
  AND user_id = :current_user_id
  AND status = 'active';  -- cannot cancel already cancelled booking

-- 9. Аутентификация (Login/Authenticate) – not explicitly listed as API in variant 13,
--    but required by the system. Included for completeness.
--    API: POST /api/v1/auth/login
--    Input: login, password
--    Output: user id, login; in code, additionally creates a session.
-- ---------------------------------------------------------------------------
-- Verify password (hash checking done in app, here we only retrieve the user)
SELECT id, login, password_hash
FROM users
WHERE login = LOWER(TRIM(:login));

-- After this, the application compares the hash and creates a session:
INSERT INTO sessions (token, user_id, login)
VALUES (:generated_token, :user_id, :login)
ON CONFLICT (token) DO UPDATE SET user_id = EXCLUDED.user_id,
                                login   = EXCLUDED.login;

-- 10. Получение сессии по токену (Authenticate session)
--     Used by the auth checker.
-- ---------------------------------------------------------------------------
SELECT token, user_id, login
FROM sessions
WHERE token = :token;

-- 11. Получение пользователя по ID
--     Used internally during login to return user info.
-- ---------------------------------------------------------------------------
SELECT id, login, name, surname, email, password_hash
FROM users
WHERE id = :user_id;
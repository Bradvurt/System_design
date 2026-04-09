
-- Создание нового пользователя
INSERT INTO users (login, password_hash, first_name, last_name, email, phone)
VALUES ('new_user', '$2b$12$w2X6lV7sP9cA8Z1vL2mN5eF3gH4jK5lM6nO4pQ8rS9tU0vW1xY2z', 'Петр', 'Петров', 'petrov@example.com', '+79031112233')
RETURNING id, login, first_name, last_name, email, phone, created_at;

-- Поиск пользователя по логину
SELECT id, login, first_name, last_name, email, phone, created_at, updated_at
FROM users
WHERE login = 'ivanov_i';

-- Поиск пользователя по маске имя и фамилии
SELECT id, login, first_name, last_name, email, phone
FROM users
WHERE first_name LIKE 'Ив%' AND last_name LIKE 'Ива%';

-- Создание услуги (отеля)
INSERT INTO hotels (name, description, city, address, stars)
VALUES (
    'Новый Отель',
    'Современный отель в центре города',
    'Москва',
    'ул. Тверская, 10',
    4
)
RETURNING id, name, city, address, stars;

-- Получение списка услуг (всех отелей)
SELECT id, name, city, address, stars, description
FROM hotels
ORDER BY name;

-- Пагинация (пример с LIMIT/OFFSET)
-- SELECT * FROM hotels ORDER BY id LIMIT 10 OFFSET 20;

-- Cоздание бронирования)
INSERT INTO bookings (user_id, hotel_id, check_in_date, check_out_date, total_price, status)
VALUES (
    1,
    3,
    '2026-07-01',
    '2026-07-07',
    56000.00,
    'active'
)
RETURNING id, user_id, hotel_id, check_in_date, check_out_date, total_price, status, created_at;

-- Получение бронирования для пользователя
SELECT 
    b.id,
    b.hotel_id,
    h.name AS hotel_name,
    h.city,
    h.address,
    b.check_in_date,
    b.check_out_date,
    b.total_price,
    b.status,
    b.created_at
FROM bookings b
JOIN hotels h ON b.hotel_id = h.id
WHERE b.user_id = 1
ORDER BY b.created_at DESC;

-- Вариант с фильтрацией по статусу (например, только активные)
-- WHERE b.user_id = 1 AND b.status = 'active'

-- Поиск отелей по городу
SELECT id, name, city, address, stars, description
FROM hotels
WHERE city = 'Москва'
ORDER BY stars DESC, name;

-- Отмена бронирования
UPDATE bookings
SET status = 'cancelled',
    updated_at = NOW()
WHERE id = 4
RETURNING id, user_id, hotel_id, status, updated_at;

-- Проверка, что бронирование принадлежит пользователю (для безопасности)
-- UPDATE bookings SET status = 'cancelled' WHERE id = 4 AND user_id = 1

-- Дополнительный запрос: Получение информации о конкретном бронировании
SELECT 
    b.id,
    b.user_id,
    u.login,
    u.first_name,
    u.last_name,
    b.hotel_id,
    h.name AS hotel_name,
    h.city,
    h.address,
    b.check_in_date,
    b.check_out_date,
    b.total_price,
    b.status,
    b.created_at,
    b.updated_at
FROM bookings b
JOIN users u ON b.user_id = u.id
JOIN hotels h ON b.hotel_id = h.id
WHERE b.id = 5;


-- Запросы для анализа EXPLAIN (будут использованы в optimization.md)

-- Пример EXPLAIN для поиска отелей по городу
EXPLAIN (ANALYZE, BUFFERS, VERBOSE)
SELECT * FROM hotels WHERE city = 'Москва';

-- Пример EXPLAIN для поиска пользователя по маске имени и фамилии
EXPLAIN (ANALYZE, BUFFERS)
SELECT * FROM users 
WHERE first_name LIKE 'Ив%' AND last_name LIKE 'Ива%';

-- Пример EXPLAIN для получения бронирований пользователя
EXPLAIN (ANALYZE, BUFFERS)
SELECT b.*, h.name 
FROM bookings b 
JOIN hotels h ON b.hotel_id = h.id 
WHERE b.user_id = 1;
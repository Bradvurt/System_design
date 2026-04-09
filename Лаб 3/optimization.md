# Оптимизация запросов в системе бронирования отелей

## Исходная ситуация

Изначально база данных содержит три таблицы:
- `users` (пользователи)
- `hotels` (отели)
- `bookings` (бронирования)

Первичные ключи (`id`) автоматически индексируются через `PRIMARY KEY`. Внешние ключи в `bookings` (`user_id`, `hotel_id`) **не индексированы**, что может приводить к полному сканированию таблиц при соединениях.

Тестовые данные: 12 пользователей, 15 отелей, 25 бронирований.

## Выявленные узкие места

Анализ API-запросов показал, что наиболее частыми будут:

| Операция API | Соответствующий SQL |
|--------------|---------------------|
| Поиск пользователя по логину | `WHERE login = ?` |
| Поиск пользователя по маске имени и фамилии | `WHERE first_name LIKE '...%' AND last_name LIKE '...%'` |
| Поиск отелей по городу | `WHERE city = ?` |
| Получение бронирований пользователя | `SELECT ... FROM bookings JOIN hotels WHERE user_id = ?` |

Без дополнительных индексов:
- `login` уже имеет `UNIQUE` ограничение → индекс существует автоматически.
- `city`, `first_name`, `last_name`, `user_id`, `hotel_id` не имеют индексов → полное сканирование таблиц.

## Созданные индексы

Для ускорения запросов были добавлены следующие индексы (см. `schema.sql`):

```sql
-- Ускорение поиска по маске имени
CREATE INDEX idx_users_first_name_pattern ON users(first_name text_pattern_ops);
CREATE INDEX idx_users_last_name_pattern ON users(last_name text_pattern_ops);
CREATE INDEX idx_users_first_last ON users(first_name, last_name);

-- Ускорение поиска отелей по городу
CREATE INDEX idx_hotels_city ON hotels(city);

-- Ускорение JOIN и WHERE по внешним ключам
CREATE INDEX idx_bookings_user_id ON bookings(user_id);
CREATE INDEX idx_bookings_hotel_id ON bookings(hotel_id);

-- Дополнительные индексы для частых фильтров
CREATE INDEX idx_bookings_user_status ON bookings(user_id, status);
CREATE INDEX idx_bookings_status ON bookings(status);
CREATE INDEX idx_bookings_dates ON bookings(check_in_date, check_out_date);
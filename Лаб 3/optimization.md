# Оптимизация запросов – Система бронирования отелей

### 1. Список отелей по городу

```sql
EXPLAIN ANALYZE
SELECT * FROM hotels WHERE LOWER(city) = LOWER('sochi');
```
```
Seq Scan on hotels  (cost=0.00..1.18 rows=1 width=...)
   Filter: (lower(city) = 'sochi'::text)
   Rows Removed by Filter: 11
 Planning Time: 0.082 ms
 Execution Time: 0.105 ms
 ```

 ### 2. Поиск пользователей по маске имени
```sql
EXPLAIN ANALYZE
SELECT * FROM users WHERE name ILIKE '%ali%';
```
```
 Seq Scan on users  (cost=0.00..1.15 rows=1 width=...)
   Filter: (name ~~* '%ali%'::text)
   Rows Removed by Filter: 11
 Planning Time: 0.074 ms
 Execution Time: 0.049 ms
 ```

 Были добавлены следующие индексы:

 ```sql
-- Быстрый поиск пользователя по логину (используется при входе и проверках)
CREATE INDEX idx_users_login_lower ON users (LOWER(login));

-- Быстрый вывод отелей по городу
CREATE INDEX idx_hotels_city_lower ON hotels (LOWER(city));

-- Быстрый поиск пользователей по имени и фамилии (поддержка ILIKE)
CREATE INDEX idx_users_name_surname ON users (name, surname);

-- Быстрый поиск активных бронирований конкретного пользователя
CREATE INDEX idx_bookings_user_status ON bookings(user_id, status)
    WHERE status = 'active';

-- Другие индексы по внешним ключам и составной индекс по датам (не показаны)
```

### 3. Список отелей по городу (с индексом)

```sql
EXPLAIN ANALYZE
SELECT * FROM hotels WHERE LOWER(city) = LOWER('sochi');
```
```
 Bitmap Heap Scan on hotels  (cost=4.20..8.31 rows=2 width=...)
   Recheck Cond: (lower(city) = 'sochi'::text)
   ->  Bitmap Index Scan on idx_hotels_city_lower  (cost=0.00..4.20 rows=2 width=0)
         Index Cond: (lower(city) = 'sochi'::text)
 Planning Time: 0.095 ms
 Execution Time: 0.035 ms
```

### 4. Поиск пользователя по логину

```sql
EXPLAIN ANALYZE
SELECT * FROM users WHERE login = LOWER('Alice');
```
```
 Index Scan using idx_users_login_lower on users  (cost=0.14..8.15 rows=1 width=...)
   Index Cond: (lower(login) = 'alice'::text)
 Planning Time: 0.081 ms
 Execution Time: 0.024 ms
```

### 5. Список активных бронирований пользователя

```sql
EXPLAIN ANALYZE
SELECT * FROM bookings WHERE user_id = 1 AND status = 'active';
```
```
 Index Only Scan using idx_bookings_user_status on bookings  (cost=0.14..8.16 rows=2 width=...)
   Index Cond: (user_id = 1)
   Heap Fetches: 0
 Planning Time: 0.065 ms
 Execution Time: 0.022 ms
```
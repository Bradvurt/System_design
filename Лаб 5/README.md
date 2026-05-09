# Система бронирования отелей — кеширование и rate limiting

### 1. Кеширование (Cache-Aside) через Redis
- `GET /api/v1/hotels` (включая фильтр по городу), TTL 60 секунд.
- `GET /api/v1/users/{user_id}/bookings`, TTL 30 секунд.

### 2. Инвалидация кеша
- Кеш отелей инвалидируется при `POST /api/v1/hotels`.
- Кеш бронирований пользователя инвалидируется при `POST /api/v1/bookings` и `DELETE /api/v1/bookings/{booking_id}`.

### 3. Rate limiting для `POST /api/v1/bookings`
- Алгоритм: Fixed Window Counter.
- Лимит: 20 запросов в минуту на пользователя.
- Счетчик запросов хранится в Redis.
- При превышении возвращается `429 Too Many Requests`.
- Заголовки: `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`.

## API

Базовый URL: `http://localhost:8080`

| Метод | Путь | Описание | Аутентификация |
|---|---|---|---|
| POST | `/api/v1/auth/register` | Регистрация пользователя | Нет |
| POST | `/api/v1/auth/login` | Вход в систему | Нет |
| GET | `/api/v1/users/by-login/{login}` | Поиск пользователя по логину | Нет |
| GET | `/api/v1/users/search?name=&surname=` | Поиск пользователя по маске | Нет |
| GET | `/api/v1/hotels?city=` | Список отелей (с фильтром) | Нет |
| POST | `/api/v1/hotels` | Создание отеля | JWT |
| POST | `/api/v1/bookings` | Создание бронирования | JWT |
| GET | `/api/v1/users/{user_id}/bookings` | Бронирования пользователя | JWT |
| DELETE | `/api/v1/bookings/{booking_id}` | Отмена бронирования | JWT |

## Запуск / проверка / остановка

```bash
docker compose up --build -d

docker compose ps

docker compose down -v
```

## API

Доступен на `http://localhost:8080`.

- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `GET /api/v1/users/by-login/{login}`
- `GET /api/v1/users/search?name=&surname=`
- `GET|POST /api/v1/hotels`
- `POST /api/v1/bookings`
- `GET /api/v1/users/{user_id}/bookings`
- `DELETE /api/v1/bookings/{booking_id}`

## Проверка rate limit

После логина используйте токен в `Authorization: Bearer <token>` и много запросов в `POST /api/v1/bookings`.  
После достижения лимита сервис вернет `429`, а также заголовки:

- `X-RateLimit-Limit`
- `X-RateLimit-Remaining`
- `X-RateLimit-Reset`

## Примеры запросов

```
curl -X POST http://localhost:8080/api/v1/hotels \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <TOKEN>" \
  -d '{
    "name": "Ocean View",
    "city": "Miami",
    "address": "456 Beach Ave",
    "description": "Sun and sea"
  }'

curl -X POST http://localhost:8080/api/v1/hotels \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <TOKEN>" \
  -d '{
    "name": "Grand Plaza",
    "city": "New York",
    "address": "123 Broadway",
    "description": "A luxury hotel in the heart of Manhattan"
  }'
```
```
curl -X POST http://localhost:8080/api/v1/bookings \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <TOKEN>" \
  -d '{
    "hotel_id": 1,
    "check_in": "2026-06-10",
    "check_out": "2026-06-15"
  }'

curl http://localhost:8080/api/v1/users/1/bookings \
  -H "Authorization: Bearer <TOKEN>"
```
# Домашнее задание №2 по курсу «Системный дизайн»
## Вариант 13: Система бронирования отелей (аналог Booking.com)

### Цель работы

Получить практические навыки разработки REST API сервиса с 
использованием принципов REST, обработкой HTTP запросов, реализацией аутентификации и документированием API. 

## Реализованные операции

- Создание пользователя
- Поиск пользователя по логину
- Поиск пользователя по маске имени и фамилии
- Создание отеля
- Получение списка отелей
- Поиск отелей по городу
- Создание бронирования
- Получение бронирований пользователя
- Отмена бронирования


## API

Базовый префикс: `/api/v1`

- `POST /users`
- `GET /users?login=...&first_name_mask=...&last_name_mask=...`
- `POST /auth/login`
- `POST /hotels`
- `GET /hotels?city=...`
- `POST /bookings` — защищён JWT
- `GET /bookings` — защищён JWT
- `DELETE /bookings/{id}` — защищён JWT

### Примеры запросов

Создание пользователя:

```bash
curl -X POST http://localhost:8080/api/v1/users \
  -H 'Content-Type: application/json' \
  -d '{"login":"alice","password":"secret","first_name":"Alice","last_name":"Ivanova"}'
```

Логин:

```bash
curl -X POST http://localhost:8080/api/v1/auth/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"alice","password":"secret"}'
```

Создание бронирования:

```bash
curl -X POST http://localhost:8080/api/v1/bookings \
  -H 'Authorization: Bearer <token>' \
  -H 'Content-Type: application/json' \
  -d '{"hotel_id":1,"check_in":"2026-04-01","check_out":"2026-04-05"}'
```

## Запуск

### Через Docker

```bash
docker compose up --build
```

Сервис поднимается на `http://localhost:8080`.

### Локально

Нужны зависимости userver и OpenSSL. Сборка через CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/hotel_booking --config=build/config.yaml
```

## Unit-тесты

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Unit-тесты проверяют:

- регистрацию пользователя;
- дублирование логина;
- логин и JWT;
- поиск пользователя по логину и маске;
- доступ без JWT к защищённым endpoint’ам;
- создание отеля и бронирования;
- получение списка отелей и бронирований;
- запрет отмены чужого бронирования;
- успешную отмену бронирования владельцем.

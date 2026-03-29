# Hotel Booking REST API (вариант 13)

Проект реализует задание варианта 13 — **система бронирования отелей**.

## Что покрыто

- Создание пользователя
- Поиск пользователя по логину
- Поиск пользователя по маске имени и фамилии
- Создание отеля
- Получение списка отелей
- Поиск отелей по городу
- Создание бронирования
- Получение бронирований пользователя
- Отмена бронирования
- JWT-аутентификация
- Защита более чем двух endpoint’ов

## API

Базовый префикс: `/api/v1`

- `POST /users`
- `GET /users?login=...&first_name_mask=...&last_name_mask=...`
- `POST /auth/login`
- `POST /hotels` — защищён JWT
- `GET /hotels?city=...`
- `POST /bookings` — защищён JWT
- `GET /bookings` — защищён JWT
- `DELETE /bookings/{id}` — защищён JWT

## Запуск

### Через Docker

```bash
docker compose up --build
```

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

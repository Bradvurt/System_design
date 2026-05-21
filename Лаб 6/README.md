# Event-Driven архитектура системы бронирования отелей

### Запуск всех сервисов
```bash
docker compose up -d
```

### Доступ к компонентам
- **API приложения**: `http://localhost:8080`
- **RabbitMQ Management UI**: `http://localhost:15672` (логин: `guest`, пароль: `guest`)
- **MongoDB**: `localhost:27017` (пользователь: `booking_admin`, пароль: `booking_password`)

## API-эндпоинты (базовый путь `/api`)

| Метод     | Эндпоинт                     | Описание                                      | Требуется токен |
|-----------|------------------------------|-----------------------------------------------|-----------------|
| `POST`    | `/auth/register`             | Регистрация нового пользователя               | Нет             |
| `POST`    | `/auth/login`                | Вход, возвращает JWT-токен                    | Нет             |
| `GET`     | `/users/by-login/{login}`    | Поиск пользователя по логину                  | Нет             |
| `GET`     | `/users/search`              | Поиск пользователей по маске имени/фамилии    | Нет             |
| `POST`    | `/hotels`                    | Добавление отеля                              | Да (`Bearer`)   |
| `GET`     | `/hotels?city=...`           | Получение списка отелей (фильтр по городу)    | Нет             |
| `POST`    | `/bookings`                  | Создание бронирования (+ событие)             | Да (`Bearer`)   |
| `GET`     | `/bookings`                  | Получение бронирований текущего пользователя  | Да (`Bearer`)   |
| `DELETE`  | `/bookings/{id}`             | Отмена бронирования (+ событие)               | Да (`Bearer`)   |

**Примечание**: Токен передаётся в заголовке `Authorization: Bearer <токен>`.

## Тестирование событий
1. Зарегистрируйте пользователя (эндпоинт `POST /api/auth/register`).
2. Авторизуйтесь (`POST /api/auth/login`) — получите токен.
3. Создайте отель (`POST /api/hotels`).
4. Создайте бронирование (`POST /api/bookings`).
5. Отмените бронирование (`DELETE /api/bookings/{id}`).
6. Просмотрите логи контейнера приложения:
   ```bash
   docker logs hotel-booking-monolith 2>&1 | grep -E "MAILER|Published"
   ```
   Вы должны увидеть сообщения о публикации и обработке событий:
   ```
   Event published: user.registered
   [MAILER] Sending welcome email to ...
   Event published: booking.created
   [MAILER] Sending booking confirmation ...
   Event published: booking.cancelled
   [MAILER] Sending cancellation email ...
   ```

## Структура проекта
- `src/domain/` — бизнес-логика, модели, работа с MongoDB
- `src/api/` — HTTP-обработчики, middleware аутентификации
- `src/events/` — компоненты продюсера и консьюмера RabbitMQ
- `src/components/` — компонент-обёртка для сервиса
- `config/` — конфигурация userver
- `docker-compose.yml` — описание инфраструктуры
- `event_driven_design.md` — описание архитектуры (на русском)
- `event_catalog.md` — каталог событий (на русском)

## Паттерн CQRS
- **Команды (Write)**: `RegisterUser`, `CreateHotel`, `CreateBooking`, `CancelBooking` изменяют состояние и публикуют события.
- **Запросы (Read)**: `FindUserByLogin`, `SearchUsers`, `ListHotels`, `GetUserBookings` только читают данные и не порождают событий.
Логическое разделение позволяет в будущем легко выделить отдельные хранилища для чтения и записи.
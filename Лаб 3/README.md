# Система бронирования отелей (вариант 13)

Данный проект представляет собой реализацию базы данных и API для системы бронирования отелей, аналогичной booking.com

## Схема базы данных

База данных состоит из трёх таблиц:

- **`users`** — пользователи системы
- **`hotels`** — информация об отелях
- **`bookings`** — бронирования, связывающие пользователей и отели

### Описание таблиц

#### `users`

| Поле          | Тип         | Ограничения                | Описание |
|---------------|-------------|----------------------------|----------|
| `id`          | BIGSERIAL   | PRIMARY KEY                | Уникальный идентификатор |
| `login`       | VARCHAR(50) | NOT NULL, UNIQUE           | Логин пользователя |
| `password_hash` | VARCHAR(255) | NOT NULL                 | Хеш пароля (bcrypt) |
| `first_name`  | VARCHAR(100) | NOT NULL                  | Имя |
| `last_name`   | VARCHAR(100) | NOT NULL                  | Фамилия |
| `email`       | VARCHAR(255) | NOT NULL, UNIQUE          | Email |
| `phone`       | VARCHAR(20)  |                           | Телефон (опционально) |
| `created_at`  | TIMESTAMPTZ | DEFAULT NOW()              | Дата создания |
| `updated_at`  | TIMESTAMPTZ | DEFAULT NOW()              | Дата обновления |

#### `hotels`

| Поле          | Тип         | Ограничения                | Описание |
|---------------|-------------|----------------------------|----------|
| `id`          | BIGSERIAL   | PRIMARY KEY                | Уникальный идентификатор |
| `name`        | VARCHAR(200)| NOT NULL                   | Название отеля |
| `description` | TEXT        |                            | Описание |
| `city`        | VARCHAR(100)| NOT NULL                   | Город |
| `address`     | VARCHAR(255)| NOT NULL                   | Адрес |
| `stars`       | SMALLINT    | CHECK (stars BETWEEN 1 AND 5) | Количество звёзд |
| `created_at`  | TIMESTAMPTZ | DEFAULT NOW()              | Дата добавления |
| `updated_at`  | TIMESTAMPTZ | DEFAULT NOW()              | Дата обновления |

#### `bookings`

| Поле             | Тип          | Ограничения                                    | Описание |
|------------------|--------------|------------------------------------------------|----------|
| `id`             | BIGSERIAL    | PRIMARY KEY                                    | Уникальный идентификатор |
| `user_id`        | BIGINT       | NOT NULL, FOREIGN KEY (`users`) ON DELETE CASCADE | Пользователь |
| `hotel_id`       | BIGINT       | NOT NULL, FOREIGN KEY (`hotels`) ON DELETE CASCADE | Отель |
| `check_in_date`  | DATE         | NOT NULL                                       | Дата заезда |
| `check_out_date` | DATE         | NOT NULL, CHECK (check_out_date > check_in_date) | Дата выезда |
| `status`         | VARCHAR(20)  | NOT NULL, DEFAULT 'active', CHECK (status IN ('active', 'cancelled', 'completed')) | Статус |
| `total_price`    | DECIMAL(10,2)| NOT NULL, CHECK (total_price >= 0)             | Стоимость |
| `created_at`     | TIMESTAMPTZ  | DEFAULT NOW()                                  | Дата создания |
| `updated_at`     | TIMESTAMPTZ  | DEFAULT NOW()                                  | Дата обновления |

### Индексы

Для оптимизации производительности созданы индексы (подробнее в [optimization.md](optimization.md)):

- `idx_users_first_name_pattern` — ускорение префиксного поиска по имени
- `idx_users_last_name_pattern` — ускорение префиксного поиска по фамилии
- `idx_hotels_city` — быстрый поиск отелей по городу
- `idx_bookings_user_id` — быстрый поиск бронирований пользователя
- `idx_bookings_hotel_id` — ускорение JOIN с отелями
- и другие

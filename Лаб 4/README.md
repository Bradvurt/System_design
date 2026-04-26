# Система бронирования отелей с MongoDB

Проект мигрирован с PostgreSQL на MongoDB с использованием документной модели данных.

## Проектирование

Проект использует документно-ориентированную базу данных MongoDB со следующими коллекциями:

- **users** - пользователи системы
- **hotels** - отели с вложенными номерами (embedded rooms)
- **bookings** - бронирования (references на users и hotels)

### Кратко про выборы embedded / references

1. **Embedded rooms в hotels**: Номера всегда запрашиваются вместе с отелем, не имеют смысла отдельно
2. **References для bookings**: Бронирования часто обновляются и могут иметь большой объем
3. **Денормализация**: Минимизирует количество запросов для типичных операцийI

### Запуск с Docker Compose

```bash
# Запуск всех сервисов
docker-compose up -d

# Проверка статуса
docker-compose ps

# Просмотр логов
docker-compose logs -f mongodb
```

### Ручное выполнение скриптов

```bash
# Подключение к MongoDB
docker exec -it hotel_booking_mongodb mongosh -u admin -p password123

# Выполнение скрипта с данными
docker exec -it hotel_booking_mongodb mongosh -u admin -p password123 /docker-entrypoint-initdb.d/01-data.js

# Выполнение CRUD запросов
docker exec -it hotel_booking_mongodb mongosh -u admin -p password123 /docker-entrypoint-initdb.d/03-queries.js

# Выполнение валидации
docker exec -it hotel_booking_mongodb mongosh -u admin -p password123 /docker-entrypoint-initdb.d/02-validation.js

# Выполнение агрегаций
docker exec -it hotel_booking_mongodb mongosh -u admin -p password123 /docker-entrypoint-initdb.d/04-aggregation.js
```
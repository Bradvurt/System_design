// Валидация схем для MongoDB
db = db.getSiblingDB('hotel_booking');

print("========== ВАЛИДАЦИЯ СХЕМ MONGODB ==========\n");

// Удаляем существующую валидацию если есть
db.runCommand({collMod: "hotels", validator: {}, validationLevel: "off"});

// Создаем валидацию для коллекции hotels
print("1. Создание валидации для коллекции hotels...");

db.createCollection("hotels_validated", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["id", "name", "city", "address", "stars", "created_at", "updated_at"],
      properties: {
        id: {
          bsonType: "long",
          description: "Уникальный идентификатор отеля (обязательное поле)"
        },
        name: {
          bsonType: "string",
          minLength: 1,
          maxLength: 200,
          description: "Название отеля (1-200 символов, обязательное)"
        },
        description: {
          bsonType: ["string", "null"],
          description: "Описание отеля (опционально)"
        },
        city: {
          bsonType: "string",
          minLength: 1,
          maxLength: 100,
          description: "Город (обязательное, 1-100 символов)"
        },
        address: {
          bsonType: "string",
          minLength: 1,
          maxLength: 255,
          description: "Адрес (обязательное, 1-255 символов)"
        },
        stars: {
          bsonType: "int",
          minimum: 1,
          maximum: 5,
          description: "Количество звезд (1-5, обязательное)"
        },
        created_by_user_id: {
          bsonType: "long",
          description: "ID создателя (опционально)"
        },
        created_at: {
          bsonType: "date",
          description: "Дата создания (обязательное)"
        },
        updated_at: {
          bsonType: "date",
          description: "Дата обновления (обязательное)"
        },
        rooms: {
          bsonType: ["array"],
          description: "Массив номеров отеля",
          items: {
            bsonType: "object",
            required: ["room_number", "type", "price_per_night", "capacity", "is_available"],
            properties: {
              room_number: {
                bsonType: "string",
                pattern: "^[A-Z0-9]+$",
                description: "Номер комнаты (только заглавные буквы и цифры)"
              },
              type: {
                bsonType: "string",
                minLength: 1,
                maxLength: 50,
                description: "Тип номера"
              },
              price_per_night: {
                bsonType: "decimal",
                minimum: 0,
                description: "Цена за ночь (>= 0)"
              },
              capacity: {
                bsonType: "int",
                minimum: 1,
                maximum: 10,
                description: "Вместимость (1-10 человек)"
              },
              is_available: {
                bsonType: "bool",
                description: "Доступность номера"
              },
              checkout_date: {
                bsonType: ["date", "null"],
                description: "Дата выселения (если занят)"
              }
            }
          }
        },
        amenities: {
          bsonType: ["array"],
          description: "Удобства отеля",
          items: {
            bsonType: "string"
          }
        }
      },
      additionalProperties: true
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

print("Валидация создана успешно!\n");

// Тестирование валидации
print("2. Тестирование валидации...\n");

// 2.1 Валидный документ
print("2.1 Попытка вставить валидный отель:");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(100),
    name: "Valid Test Hotel",
    city: "Test City",
    address: "123 Test Street",
    stars: 4,
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "101",
        type: "Standard",
        price_per_night: NumberDecimal("100.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  });
  print("✓ Успешно вставлен");
} catch (e) {
  print("✗ Ошибка: " + e.message);
}

// 2.2 Невалидный документ - отсутствует обязательное поле
print("\n2.2 Попытка вставить отель без обязательного поля 'city':");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(101),
    name: "Invalid Hotel No City",
    address: "123 Test Street",
    stars: 4,
    created_at: new Date(),
    updated_at: new Date()
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

// 2.3 Невалидный документ - неправильный тип данных
print("\n2.3 Попытка вставить отель с неправильным типом поля 'stars' (строка вместо числа):");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(102),
    name: "Invalid Stars Type",
    city: "Test City",
    address: "123 Test Street",
    stars: "five", // Должно быть число
    created_at: new Date(),
    updated_at: new Date()
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

// 2.4 Невалидный документ - выход за границы допустимых значений
print("\n2.4 Попытка вставить отель с 'stars' = 6 (максимум 5):");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(103),
    name: "Invalid Stars Range",
    city: "Test City",
    address: "123 Test Street",
    stars: 6,
    created_at: new Date(),
    updated_at: new Date()
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

// 2.5 Невалидный вложенный документ в массиве rooms
print("\n2.5 Попытка вставить отель с невалидным номером (неправильный формат room_number):");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(104),
    name: "Invalid Room Format",
    city: "Test City",
    address: "123 Test Street",
    stars: 3,
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "abc-123", // Должны быть только заглавные буквы и цифры
        type: "Standard",
        price_per_night: NumberDecimal("100.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

// 2.6 Невалидный документ - отрицательная цена
print("\n2.6 Попытка вставить отель с отрицательной ценой номера:");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(105),
    name: "Invalid Price",
    city: "Test City",
    address: "123 Test Street",
    stars: 3,
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "101",
        type: "Standard",
        price_per_night: NumberDecimal("-50.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

// 2.7 Невалидный документ - превышение вместимости
print("\n2.7 Попытка вставить отель с вместимостью номера > 10:");
try {
  db.hotels_validated.insertOne({
    id: NumberLong(106),
    name: "Invalid Capacity",
    city: "Test City",
    address: "123 Test Street",
    stars: 3,
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "101",
        type: "Dormitory",
        price_per_night: NumberDecimal("50.00"),
        capacity: 20,
        is_available: true,
        checkout_date: null
      }
    ]
  });
  print("✗ Документ вставлен (НЕ ОЖИДАЛОСЬ)");
} catch (e) {
  print("✓ Ошибка валидации: " + e.message.split(":")[0]);
}

print("\n3. Статистика валидации:");
print("Успешно вставлено документов: " + db.hotels_validated.count());

// Показываем информацию о валидации
print("\n4. Информация о валидации коллекции:");
const collInfo = db.getCollectionInfos({name: "hotels_validated"})[0];
print(JSON.stringify(collInfo.options.validator, null, 2));

print("\n========== ТЕСТИРОВАНИЕ ВАЛИДАЦИИ ЗАВЕРШЕНО ==========");
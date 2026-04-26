// MongoDB запросы для всех CRUD операций
db = db.getSiblingDB('hotel_booking');

print("========== CRUD ОПЕРАЦИИ ДЛЯ MONGODB ==========\n");

// ========== CREATE (ВСТАВКА) ==========
print("1. CREATE - Вставка документов\n");

// 1.1 Вставка одного пользователя
print("1.1 Вставка нового пользователя:");
db.users.insertOne({
  id: NumberLong(11),
  login: "maria_garcia",
  first_name: "Maria",
  last_name: "Garcia",
  email: "maria.g@example.com",
  phone: "+1222333444",
  password_hash: "k1l2m3n4o5p6q7r8",
  created_at: new Date(),
  updated_at: new Date()
});
print("Пользователь создан: " + db.users.findOne({id: NumberLong(11)}).login);

// 1.2 Вставка нескольких отелей
print("\n1.2 Вставка нескольких отелей:");
db.hotels.insertMany([
  {
    id: NumberLong(11),
    name: "Alpine Ski Resort",
    description: "Горнолыжный курорт с прямым доступом к трассам",
    city: "Aspen",
    address: "123 Ski Way, Aspen, CO 81611",
    stars: 4,
    created_by_user_id: NumberLong(1),
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "101",
        type: "Ski-in/Ski-out",
        price_per_night: NumberDecimal("450.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  },
  {
    id: NumberLong(12),
    name: "Beachfront Paradise",
    description: "Отель прямо на пляже с частной территорией",
    city: "Honolulu",
    address: "456 Beach Rd, Honolulu, HI 96815",
    stars: 4,
    created_by_user_id: NumberLong(2),
    created_at: new Date(),
    updated_at: new Date(),
    rooms: [
      {
        room_number: "A1",
        type: "Oceanfront",
        price_per_night: NumberDecimal("380.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  }
]);
print("Добавлено отелей: " + db.hotels.count({id: {$in: [NumberLong(11), NumberLong(12)]}}));

// ========== READ (ПОИСК) ==========
print("\n========== 2. READ - Поиск документов ==========\n");

// 2.1 Поиск с $eq (равно)
print("2.1 Поиск пользователей из Нью-Йорка (по email домену):");
db.users.find({email: {$regex: "@example\\.com$"}}).forEach(u => print(" - " + u.login));

// 2.2 Поиск с $ne (не равно)
print("\n2.2 Активные бронирования (status != 'cancelled'):");
db.bookings.find(
  {status: {$ne: "cancelled"}},
  {id: 1, user_id: 1, hotel_id: 1, status: 1, _id: 0}
).limit(3).forEach(b => print(" - Booking " + b.id + ": " + b.status));

// 2.3 Поиск с $gt (больше) и $lt (меньше)
print("\n2.3 Отели с ценой номера от 200 до 400:");
db.hotels.find({
  "rooms.price_per_night": {
    $gt: NumberDecimal("200.00"),
    $lt: NumberDecimal("400.00")
  }
}, {name: 1, "rooms.price_per_night": 1, _id: 0}).limit(3).forEach(h => {
  print(" - " + h.name);
});

// 2.4 Поиск с $in (в списке)
print("\n2.4 Отели в городах New York, Miami или Las Vegas:");
db.hotels.find(
  {city: {$in: ["New York", "Miami", "Las Vegas"]}},
  {name: 1, city: 1, stars: 1, _id: 0}
).forEach(h => print(" - " + h.name + " (" + h.city + ") " + h.stars + "*"));

// 2.5 Поиск с $and
print("\n2.5 5-звездочные отели в Нью-Йорке (используя $and):");
db.hotels.find({
  $and: [
    {stars: {$eq: 5}},
    {city: {$eq: "New York"}}
  ]
}, {name: 1, city: 1, stars: 1, _id: 0}).forEach(h => print(" - " + h.name));

// 2.6 Поиск с $or
print("\n2.6 Отели с 5 звездами ИЛИ в Чикаго:");
db.hotels.find({
  $or: [
    {stars: 5},
    {city: "Chicago"}
  ]
}, {name: 1, city: 1, stars: 1, _id: 0}).forEach(h => print(" - " + h.name + " (" + h.city + ") " + h.stars + "*"));

// 2.7 Поиск с $regex (поиск по шаблону)
print("\n2.7 Отели с 'Resort' в названии:");
db.hotels.find(
  {name: {$regex: "Resort", $options: "i"}},
  {name: 1, _id: 0}
).forEach(h => print(" - " + h.name));

// 2.8 Поиск с проекцией (выборка определенных полей)
print("\n2.8 Пользователи (только имя, фамилия и email):");
db.users.find(
  {},
  {first_name: 1, last_name: 1, email: 1, _id: 0}
).limit(3).forEach(u => print(" - " + u.first_name + " " + u.last_name + " <" + u.email + ">"));

// 2.9 Поиск во вложенных документах (номера)
print("\n2.9 Доступные номера типа 'Suite':");
db.hotels.find(
  {"rooms.type": "Suite", "rooms.is_available": true},
  {name: 1, "rooms.$": 1, _id: 0}
).forEach(h => print(" - " + h.name + ": " + h.rooms[0].room_number + " ($" + h.rooms[0].price_per_night + ")"));

// 2.10 Поиск с сортировкой и лимитом
print("\n2.10 Топ-3 самых дорогих отеля:");
db.hotels.find(
  {},
  {name: 1, "rooms.price_per_night": 1, _id: 0}
).sort({"rooms.price_per_night": -1}).limit(3).forEach(h => {
  const maxPrice = Math.max(...h.rooms.map(r => parseFloat(r.price_per_night)));
  print(" - " + h.name + ": $" + maxPrice);
});

// ========== UPDATE (ОБНОВЛЕНИЕ) ==========
print("\n========== 3. UPDATE - Обновление документов ==========\n");

// 3.1 Обновление одного поля ($set)
print("3.1 Обновление email пользователя (John Doe):");
const beforeUpdate = db.users.findOne({id: NumberLong(1)});
db.users.updateOne(
  {id: NumberLong(1)},
  {$set: {email: "john.doe.updated@example.com", updated_at: new Date()}}
);
const afterUpdate = db.users.findOne({id: NumberLong(1)});
print(" Было: " + beforeUpdate.email);
print(" Стало: " + afterUpdate.email);

// 3.2 Обновление с $inc (инкремент)
print("\n3.2 Добавление поля 'loyalty_points' и его увеличение:");
db.users.updateOne(
  {id: NumberLong(1)},
  {$inc: {loyalty_points: 100}}
);
const userWithPoints = db.users.findOne({id: NumberLong(1)});
print(" Loyalty points: " + userWithPoints.loyalty_points);

// 3.3 Обновление нескольких документов ($mul)
print("\n3.3 Повышение цен на 10% для отелей с рейтингом ниже 4:");
db.hotels.updateMany(
  {stars: {$lt: 4}},
  {$mul: {"rooms.$[].price_per_night": NumberDecimal("1.10")}}
);
print(" Цены обновлены для " + db.hotels.count({stars: {$lt: 4}}) + " отелей");

// 3.4 Обновление с $push (добавление в массив)
print("\n3.4 Добавление нового номера в отель (с использованием $push):");
db.hotels.updateOne(
  {id: NumberLong(1)},
  {$push: {
    rooms: {
      room_number: "401",
      type: "Presidential Suite",
      price_per_night: NumberDecimal("800.00"),
      capacity: 4,
      is_available: true,
      checkout_date: null
    }
  }}
);
print(" Новый номер добавлен в Grand Plaza Hotel");

// 3.5 Обновление с $pull (удаление из массива)
print("\n3.5 Удаление номера из отеля (с использованием $pull):");
db.hotels.updateOne(
  {id: NumberLong(1)},
  {$pull: {rooms: {room_number: "401"}}}
);
print(" Номер 401 удален из Grand Plaza Hotel");

// 3.6 Обновление с $addToSet (добавление, если не существует)
print("\n3.6 Добавление уникального тега отелю (с использованием $addToSet):");
db.hotels.updateOne(
  {id: NumberLong(1)},
  {$addToSet: {amenities: "Swimming Pool"}}
);
db.hotels.updateOne(
  {id: NumberLong(1)},
  {$addToSet: {amenities: "Swimming Pool"}} // Не добавит дубликат
);
const hotelWithAmenities = db.hotels.findOne({id: NumberLong(1)});
print(" Amenities: " + hotelWithAmenities.amenities.join(", "));

// 3.7 Обновление с upsert
print("\n3.7 Обновление с upsert (создать если не существует):");
db.hotels.updateOne(
  {id: NumberLong(99)},
  {$set: {
    id: NumberLong(99),
    name: "New Upsert Hotel",
    city: "Portland",
    stars: 3,
    created_at: new Date(),
    updated_at: new Date()
  }},
  {upsert: true}
);
print(" Отель создан через upsert: " + (db.hotels.findOne({id: NumberLong(99)}) ? "Да" : "Нет"));

// 3.8 Обновление статуса бронирования
print("\n3.8 Обновление статуса бронирования на 'completed':");
db.bookings.updateOne(
  {id: NumberLong(1)},
  {$set: {status: "completed", updated_at: new Date()}}
);
print(" Статус обновлен: " + db.bookings.findOne({id: NumberLong(1)}).status);

// ========== DELETE (УДАЛЕНИЕ) ==========
print("\n========== 4. DELETE - Удаление документов ==========\n");

// 4.1 Удаление одного документа
print("4.1 Удаление тестового пользователя:");
const deleteResult = db.users.deleteOne({id: NumberLong(11)});
print(" Удалено пользователей: " + deleteResult.deletedCount);

// 4.2 Удаление нескольких документов
print("\n4.2 Удаление всех завершенных бронирований:");
const deleteManyResult = db.bookings.deleteMany({status: "completed"});
print(" Удалено бронирований: " + deleteManyResult.deletedCount);

// 4.3 Удаление с условием
print("\n4.3 Удаление отелей без номеров:");
const deleteHotelsResult = db.hotels.deleteMany({
  $or: [
    {rooms: {$exists: false}},
    {rooms: {$size: 0}}
  ]
});
print(" Удалено отелей: " + deleteHotelsResult.deletedCount);

// ========== ДОПОЛНИТЕЛЬНЫЕ ОПЕРАЦИИ ==========
print("\n========== 5. ДОПОЛНИТЕЛЬНЫЕ ОПЕРАЦИИ ==========\n");

// 5.1 Поиск с $elemMatch
print("5.1 Поиск отелей с доступными номерами вместимостью >= 4 (используя $elemMatch):");
db.hotels.find({
  rooms: {
    $elemMatch: {
      capacity: {$gte: 4},
      is_available: true
    }
  }
}, {name: 1, "rooms.$": 1, _id: 0}).forEach(h => {
  const room = h.rooms.find(r => r.capacity >= 4 && r.is_available);
  print(" - " + h.name + ": " + room.type + " (вместимость: " + room.capacity + ")");
});

// 5.2 Использование $exists
print("\n5.2 Пользователи с указанным телефоном:");
db.users.find(
  {phone: {$exists: true, $ne: ""}},
  {login: 1, phone: 1, _id: 0}
).limit(3).forEach(u => print(" - " + u.login + ": " + u.phone));

// 5.3 Текстовый поиск
print("\n5.3 Текстовый поиск пользователей (имя 'John' или 'Jane'):");
db.users.find(
  {$text: {$search: "John Jane"}},
  {first_name: 1, last_name: 1, _id: 0}
).forEach(u => print(" - " + u.first_name + " " + u.last_name));

print("\n========== ВСЕ ОПЕРАЦИИ ВЫПОЛНЕНЫ УСПЕШНО ==========");
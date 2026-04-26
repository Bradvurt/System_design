// Aggregation Pipeline для сложных запросов
db = db.getSiblingDB('hotel_booking');

print("========== AGGREGATION PIPELINE ПРИМЕРЫ ==========\n");

// 1. Статистика по отелям: количество бронирований, доход, загрузка
print("1. Статистика по отелям с бронированиями:");
db.bookings.aggregate([
  // Stage 1: Фильтруем только активные и завершенные бронирования
  {
    $match: {
      status: { $in: ["active", "completed"] }
    }
  },
  // Stage 2: Группируем по отелю
  {
    $group: {
      _id: "$hotel_id",
      total_bookings: { $sum: 1 },
      total_revenue: { $sum: "$total_price" },
      avg_booking_value: { $avg: "$total_price" },
      unique_users: { $addToSet: "$user_id" }
    }
  },
  // Stage 3: Добавляем информацию об отеле через $lookup
  {
    $lookup: {
      from: "hotels",
      localField: "_id",
      foreignField: "id",
      as: "hotel_info"
    }
  },
  // Stage 4: Разворачиваем массив с информацией об отеле
  {
    $unwind: "$hotel_info"
  },
  // Stage 5: Проецируем нужные поля
  {
    $project: {
      hotel_id: "$_id",
      hotel_name: "$hotel_info.name",
      city: "$hotel_info.city",
      stars: "$hotel_info.stars",
      total_bookings: 1,
      total_revenue: { $round: ["$total_revenue", 2] },
      avg_booking_value: { $round: ["$avg_booking_value", 2] },
      unique_users_count: { $size: "$unique_users" }
    }
  },
  // Stage 6: Сортируем по доходу
  {
    $sort: { total_revenue: -1 }
  }
]).forEach(r => {
  print(` - ${r.hotel_name} (${r.city}):`);
  print(`   Бронирований: ${r.total_bookings}`);
  print(`   Доход: $${r.total_revenue}`);
  print(`   Средний чек: $${r.avg_booking_value}`);
  print(`   Уникальных гостей: ${r.unique_users_count}`);
});

// 2. Анализ загрузки отелей по месяцам
print("\n2. Загрузка отелей по месяцам:");
db.bookings.aggregate([
  // Stage 1: Добавляем месяц из check_in
  {
    $addFields: {
      booking_month: { $month: "$check_in" },
      booking_year: { $year: "$check_in" },
      nights: {
        $divide: [
          { $subtract: ["$check_out", "$check_in"] },
          1000 * 60 * 60 * 24
        ]
      }
    }
  },
  // Stage 2: Группируем по отелю и месяцу
  {
    $group: {
      _id: {
        hotel_id: "$hotel_id",
        year: "$booking_year",
        month: "$booking_month"
      },
      total_nights: { $sum: "$nights" },
      total_bookings: { $sum: 1 },
      total_revenue: { $sum: "$total_price" }
    }
  },
  // Stage 3: Добавляем информацию об отеле
  {
    $lookup: {
      from: "hotels",
      localField: "_id.hotel_id",
      foreignField: "id",
      as: "hotel"
    }
  },
  // Stage 4: Разворачиваем информацию об отеле
  {
    $unwind: "$hotel"
  },
  // Stage 5: Проецируем результаты
  {
    $project: {
      hotel_name: "$hotel.name",
      period: {
        $concat: [
          { $toString: "$_id.year" },
          "-",
          { $toString: "$_id.month" }
        ]
      },
      total_nights: 1,
      total_bookings: 1,
      total_revenue: { $round: ["$total_revenue", 2] },
      occupancy_rate: {
        $multiply: [
          { $divide: ["$total_nights", { $multiply: [30, { $size: "$hotel.rooms" }] }] },
          100
        ]
      }
    }
  },
  // Stage 6: Сортируем
  {
    $sort: {
      "hotel_name": 1,
      "period": 1
    }
  }
]).limit(5).forEach(r => {
  print(` - ${r.hotel_name} (${r.period}):`);
  print(`   Ночей: ${r.total_nights}`);
  print(`   Бронирований: ${r.total_bookings}`);
  print(`   Доход: $${r.total_revenue}`);
  print(`   Загрузка: ${r.occupancy_rate.toFixed(1)}%`);
});

// 3. Анализ пользователей с наибольшим количеством бронирований
print("\n3. Топ-5 пользователей по количеству бронирований:");
db.bookings.aggregate([
  // Stage 1: Группируем по пользователю
  {
    $group: {
      _id: "$user_id",
      total_bookings: { $sum: 1 },
      total_spent: { $sum: "$total_price" },
      bookings: {
        $push: {
          hotel_id: "$hotel_id",
          check_in: "$check_in",
          check_out: "$check_out",
          status: "$status",
          price: "$total_price"
        }
      }
    }
  },
  // Stage 2: Добавляем информацию о пользователе
  {
    $lookup: {
      from: "users",
      localField: "_id",
      foreignField: "id",
      as: "user"
    }
  },
  // Stage 3: Разворачиваем информацию о пользователе
  {
    $unwind: "$user"
  },
  // Stage 4: Считаем статистику по статусам бронирований
  {
    $addFields: {
      active_bookings: {
        $size: {
          $filter: {
            input: "$bookings",
            as: "booking",
            cond: { $eq: ["$$booking.status", "active"] }
          }
        }
      },
      cancelled_bookings: {
        $size: {
          $filter: {
            input: "$bookings",
            as: "booking",
            cond: { $eq: ["$$booking.status", "cancelled"] }
          }
        }
      },
      completed_bookings: {
        $size: {
          $filter: {
            input: "$bookings",
            as: "booking",
            cond: { $eq: ["$$booking.status", "completed"] }
          }
        }
      }
    }
  },
  // Stage 5: Проецируем финальные поля
  {
    $project: {
      user_name: { $concat: ["$user.first_name", " ", "$user.last_name"] },
      email: "$user.email",
      total_bookings: 1,
      active_bookings: 1,
      cancelled_bookings: 1,
      completed_bookings: 1,
      total_spent: { $round: ["$total_spent", 2] },
      avg_booking_value: { $round: [{ $divide: ["$total_spent", "$total_bookings"] }, 2] }
    }
  },
  // Stage 6: Сортируем по количеству бронирований
  {
    $sort: { total_bookings: -1 }
  },
  // Stage 7: Ограничиваем результат
  {
    $limit: 5
  }
]).forEach(u => {
  print(` - ${u.user_name} (${u.email}):`);
  print(`   Всего бронирований: ${u.total_bookings}`);
  print(`   Активных: ${u.active_bookings}`);
  print(`   Отмененных: ${u.cancelled_bookings}`);
  print(`   Завершенных: ${u.completed_bookings}`);
  print(`   Потрачено всего: $${u.total_spent}`);
  print(`   Средний чек: $${u.avg_booking_value}`);
});

// 4. Поиск свободных номеров на заданные даты
print("\n4. Поиск свободных номеров на 20-25 февраля 2024:");
const check_in = new Date("2024-02-20T14:00:00Z");
const check_out = new Date("2024-02-25T11:00:00Z");

db.hotels.aggregate([
  // Stage 1: Разворачиваем массив номеров
  {
    $unwind: "$rooms"
  },
  // Stage 2: Фильтруем только доступные номера
  {
    $match: {
      "rooms.is_available": true
    }
  },
  // Stage 3: Исключаем номера с конфликтующими бронированиями
  {
    $lookup: {
      from: "bookings",
      let: {
        hotel_id: "$id",
        room_number: "$rooms.room_number"
      },
      pipeline: [
        {
          $match: {
            $expr: {
              $and: [
                { $eq: ["$hotel_id", "$$hotel_id"] },
                { $eq: ["$room_number", "$$room_number"] },
                { $eq: ["$status", "active"] },
                {
                  $not: {
                    $or: [
                      { $gte: ["$check_in", check_out] },
                      { $lte: ["$check_out", check_in] }
                    ]
                  }
                }
              ]
            }
          }
        }
      ],
      as: "conflicting_bookings"
    }
  },
  // Stage 4: Оставляем только номера без конфликтов
  {
    $match: {
      conflicting_bookings: { $size: 0 }
    }
  },
  // Stage 5: Группируем обратно по отелю
  {
    $group: {
      _id: "$id",
      hotel_name: { $first: "$name" },
      city: { $first: "$city" },
      stars: { $first: "$stars" },
      available_rooms: {
        $push: {
          room_number: "$rooms.room_number",
          type: "$rooms.type",
          price_per_night: "$rooms.price_per_night",
          capacity: "$rooms.capacity"
        }
      }
    }
  },
  // Stage 6: Сортируем по звездам и названию
  {
    $sort: {
      stars: -1,
      hotel_name: 1
    }
  }
]).forEach(h => {
  print(` - ${h.hotel_name} (${h.city}) ${h.stars}*:`);
  h.available_rooms.forEach(r => {
    print(`   Номер ${r.room_number}: ${r.type}, $${r.price_per_night}/ночь, вместимость: ${r.capacity}`);
  });
});

// 5. Анализ популярности типов номеров
print("\n5. Популярность типов номеров:");
db.bookings.aggregate([
  // Stage 1: Добавляем информацию об отеле и номере
  {
    $lookup: {
      from: "hotels",
      localField: "hotel_id",
      foreignField: "id",
      as: "hotel"
    }
  },
  // Stage 2: Разворачиваем отель
  {
    $unwind: "$hotel"
  },
  // Stage 3: Находим конкретный номер в массиве rooms
  {
    $addFields: {
      room_details: {
        $first: {
          $filter: {
            input: "$hotel.rooms",
            as: "room",
            cond: { $eq: ["$$room.room_number", "$room_number"] }
          }
        }
      }
    }
  },
  // Stage 4: Группируем по типу номера
  {
    $group: {
      _id: "$room_details.type",
      total_bookings: { $sum: 1 },
      total_revenue: { $sum: "$total_price" },
      avg_price: { $avg: "$room_details.price_per_night" }
    }
  },
  // Stage 5: Сортируем по популярности
  {
    $sort: { total_bookings: -1 }
  },
  // Stage 6: Проецируем результаты
  {
    $project: {
      room_type: "$_id",
      total_bookings: 1,
      total_revenue: { $round: ["$total_revenue", 2] },
      avg_price: { $round: ["$avg_price", 2] },
      revenue_percentage: {
        $round: [
          {
            $multiply: [
              { $divide: ["$total_revenue", { $sum: "$total_revenue" }] },
              100
            ]
          },
          1
        ]
      }
    }
  }
]).forEach(r => {
  print(` - ${r.room_type}:`);
  print(`   Бронирований: ${r.total_bookings}`);
  print(`   Доход: $${r.total_revenue}`);
  print(`   Средняя цена: $${r.avg_price}`);
  print(`   Доля в доходе: ${r.revenue_percentage}%`);
});

print("\n========== AGGREGATION PIPELINE ЗАВЕРШЕН ==========");
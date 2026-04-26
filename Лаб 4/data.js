// Тестовые данные для MongoDB

// Переключение на базу данных
db = db.getSiblingDB('hotel_booking');

// Очистка коллекций перед вставкой
db.users.drop();
db.hotels.drop();
db.bookings.drop();
db.sessions.drop();

// Создание индексов
db.users.createIndex({ "id": 1 }, { unique: true });
db.users.createIndex({ "login": 1 }, { unique: true });
db.users.createIndex({ "email": 1 }, { unique: true });
db.users.createIndex({ "first_name": "text", "last_name": "text" });

db.hotels.createIndex({ "id": 1 }, { unique: true });
db.hotels.createIndex({ "city": 1 });
db.hotels.createIndex({ "name": 1 });
db.hotels.createIndex({ "stars": 1 });

db.bookings.createIndex({ "id": 1 }, { unique: true });
db.bookings.createIndex({ "user_id": 1 });
db.bookings.createIndex({ "hotel_id": 1 });
db.bookings.createIndex({ "check_in": 1, "check_out": 1 });
db.bookings.createIndex({ "status": 1 });

db.sessions.createIndex({ "token": 1 }, { unique: true });
db.sessions.createIndex({ "created_at": 1 }, { expireAfterSeconds: 86400 }); // TTL 24 часа

// 1. Вставка пользователей (минимум 10)
db.users.insertMany([
  {
    id: NumberLong(1),
    login: "john_doe",
    first_name: "John",
    last_name: "Doe",
    email: "john.doe@example.com",
    phone: "+1234567890",
    password_hash: "a1b2c3d4e5f6g7h8",
    created_at: new Date("2024-01-01T10:00:00Z"),
    updated_at: new Date("2024-01-01T10:00:00Z")
  },
  {
    id: NumberLong(2),
    login: "jane_smith",
    first_name: "Jane",
    last_name: "Smith",
    email: "jane.smith@example.com",
    phone: "+0987654321",
    password_hash: "b2c3d4e5f6g7h8i9",
    created_at: new Date("2024-01-02T11:30:00Z"),
    updated_at: new Date("2024-01-02T11:30:00Z")
  },
  {
    id: NumberLong(3),
    login: "bob_wilson",
    first_name: "Bob",
    last_name: "Wilson",
    email: "bob.wilson@example.com",
    phone: "+1122334455",
    password_hash: "c3d4e5f6g7h8i9j0",
    created_at: new Date("2024-01-03T09:15:00Z"),
    updated_at: new Date("2024-01-03T09:15:00Z")
  },
  {
    id: NumberLong(4),
    login: "alice_johnson",
    first_name: "Alice",
    last_name: "Johnson",
    email: "alice.j@example.com",
    phone: "+1567890123",
    password_hash: "d4e5f6g7h8i9j0k1",
    created_at: new Date("2024-01-04T14:20:00Z"),
    updated_at: new Date("2024-01-04T14:20:00Z")
  },
  {
    id: NumberLong(5),
    login: "charlie_brown",
    first_name: "Charlie",
    last_name: "Brown",
    email: "charlie.b@example.com",
    phone: "+1345678901",
    password_hash: "e5f6g7h8i9j0k1l2",
    created_at: new Date("2024-01-05T08:45:00Z"),
    updated_at: new Date("2024-01-05T08:45:00Z")
  },
  {
    id: NumberLong(6),
    login: "diana_prince",
    first_name: "Diana",
    last_name: "Prince",
    email: "diana.p@example.com",
    phone: "+1789012345",
    password_hash: "f6g7h8i9j0k1l2m3",
    created_at: new Date("2024-01-06T16:10:00Z"),
    updated_at: new Date("2024-01-06T16:10:00Z")
  },
  {
    id: NumberLong(7),
    login: "edward_norton",
    first_name: "Edward",
    last_name: "Norton",
    email: "ed.norton@example.com",
    phone: "+1890123456",
    password_hash: "g7h8i9j0k1l2m3n4",
    created_at: new Date("2024-01-07T12:00:00Z"),
    updated_at: new Date("2024-01-07T12:00:00Z")
  },
  {
    id: NumberLong(8),
    login: "fiona_green",
    first_name: "Fiona",
    last_name: "Green",
    email: "fiona.g@example.com",
    phone: "+1901234567",
    password_hash: "h8i9j0k1l2m3n4o5",
    created_at: new Date("2024-01-08T10:30:00Z"),
    updated_at: new Date("2024-01-08T10:30:00Z")
  },
  {
    id: NumberLong(9),
    login: "george_martin",
    first_name: "George",
    last_name: "Martin",
    email: "george.m@example.com",
    phone: "+1012345678",
    password_hash: "i9j0k1l2m3n4o5p6",
    created_at: new Date("2024-01-09T13:45:00Z"),
    updated_at: new Date("2024-01-09T13:45:00Z")
  },
  {
    id: NumberLong(10),
    login: "helen_carter",
    first_name: "Helen",
    last_name: "Carter",
    email: "helen.c@example.com",
    phone: "+1111111111",
    password_hash: "j0k1l2m3n4o5p6q7",
    created_at: new Date("2024-01-10T15:20:00Z"),
    updated_at: new Date("2024-01-10T15:20:00Z")
  }
]);

// 2. Вставка отелей с номерами (минимум 10)
db.hotels.insertMany([
  {
    id: NumberLong(1),
    name: "Grand Plaza Hotel",
    description: "Роскошный отель в центре города с панорамным видом",
    city: "New York",
    address: "123 Broadway, Manhattan, NY 10001",
    stars: 5,
    created_by_user_id: NumberLong(1),
    created_at: new Date("2024-01-15T09:00:00Z"),
    updated_at: new Date("2024-01-15T09:00:00Z"),
    rooms: [
      {
        room_number: "101",
        type: "Standard",
        price_per_night: NumberDecimal("150.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "102",
        type: "Standard",
        price_per_night: NumberDecimal("150.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-20T11:00:00Z")
      },
      {
        room_number: "201",
        type: "Deluxe",
        price_per_night: NumberDecimal("250.00"),
        capacity: 3,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "301",
        type: "Suite",
        price_per_night: NumberDecimal("500.00"),
        capacity: 4,
        is_available: true,
        checkout_date: null
      }
    ]
  },
  {
    id: NumberLong(2),
    name: "Seaside Resort",
    description: "Прекрасный курорт на берегу океана",
    city: "Miami",
    address: "456 Ocean Drive, Miami Beach, FL 33139",
    stars: 4,
    created_by_user_id: NumberLong(1),
    created_at: new Date("2024-01-16T10:00:00Z"),
    updated_at: new Date("2024-01-16T10:00:00Z"),
    rooms: [
      {
        room_number: "10A",
        type: "Ocean View",
        price_per_night: NumberDecimal("300.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "10B",
        type: "Ocean View",
        price_per_night: NumberDecimal("300.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-22T11:00:00Z")
      },
      {
        room_number: "20A",
        type: "Garden View",
        price_per_night: NumberDecimal("200.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  },
  {
    id: NumberLong(3),
    name: "Mountain Lodge",
    description: "Уютный отель в горах для любителей природы",
    city: "Denver",
    address: "789 Pine Street, Denver, CO 80202",
    stars: 3,
    created_by_user_id: NumberLong(2),
    created_at: new Date("2024-01-17T11:00:00Z"),
    updated_at: new Date("2024-01-17T11:00:00Z"),
    rooms: [
      {
        room_number: "1",
        type: "Standard",
        price_per_night: NumberDecimal("100.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "2",
        type: "Standard",
        price_per_night: NumberDecimal("100.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "5",
        type: "Family",
        price_per_night: NumberDecimal("180.00"),
        capacity: 4,
        is_available: false,
        checkout_date: new Date("2024-02-18T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(4),
    name: "Business Hotel Downtown",
    description: "Идеальное место для деловых поездок",
    city: "Chicago",
    address: "321 Michigan Ave, Chicago, IL 60601",
    stars: 4,
    created_by_user_id: NumberLong(2),
    created_at: new Date("2024-01-18T12:00:00Z"),
    updated_at: new Date("2024-01-18T12:00:00Z"),
    rooms: [
      {
        room_number: "401",
        type: "Business",
        price_per_night: NumberDecimal("220.00"),
        capacity: 1,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "402",
        type: "Business",
        price_per_night: NumberDecimal("220.00"),
        capacity: 1,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "501",
        type: "Executive",
        price_per_night: NumberDecimal("350.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-25T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(5),
    name: "Historic Inn",
    description: "Отель в историческом здании с уникальной атмосферой",
    city: "Boston",
    address: "567 Beacon Street, Boston, MA 02115",
    stars: 3,
    created_by_user_id: NumberLong(3),
    created_at: new Date("2024-01-19T13:00:00Z"),
    updated_at: new Date("2024-01-19T13:00:00Z"),
    rooms: [
      {
        room_number: "1A",
        type: "Classic",
        price_per_night: NumberDecimal("120.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "2B",
        type: "Classic",
        price_per_night: NumberDecimal("120.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-21T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(6),
    name: "Sunset Boulevard Hotel",
    description: "Гламурный отель в сердце Голливуда",
    city: "Los Angeles",
    address: "8900 Sunset Blvd, West Hollywood, CA 90069",
    stars: 5,
    created_by_user_id: NumberLong(3),
    created_at: new Date("2024-01-20T14:00:00Z"),
    updated_at: new Date("2024-01-20T14:00:00Z"),
    rooms: [
      {
        room_number: "PH1",
        type: "Penthouse",
        price_per_night: NumberDecimal("1200.00"),
        capacity: 4,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "701",
        type: "Luxury",
        price_per_night: NumberDecimal("450.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "702",
        type: "Luxury",
        price_per_night: NumberDecimal("450.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-28T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(7),
    name: "Desert Oasis Resort",
    description: "Роскошный курорт в пустыне с спа-центром",
    city: "Las Vegas",
    address: "3131 Las Vegas Blvd S, Las Vegas, NV 89109",
    stars: 5,
    created_by_user_id: NumberLong(4),
    created_at: new Date("2024-01-21T15:00:00Z"),
    updated_at: new Date("2024-01-21T15:00:00Z"),
    rooms: [
      {
        room_number: "1501",
        type: "Suite",
        price_per_night: NumberDecimal("600.00"),
        capacity: 3,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "1502",
        type: "Suite",
        price_per_night: NumberDecimal("600.00"),
        capacity: 3,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "2001",
        type: "Presidential",
        price_per_night: NumberDecimal("2000.00"),
        capacity: 6,
        is_available: false,
        checkout_date: new Date("2024-02-26T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(8),
    name: "Harbor View Hotel",
    description: "Отель с видом на гавань и отличным рестораном",
    city: "San Francisco",
    address: "499 Jefferson St, San Francisco, CA 94109",
    stars: 4,
    created_by_user_id: NumberLong(4),
    created_at: new Date("2024-01-22T16:00:00Z"),
    updated_at: new Date("2024-01-22T16:00:00Z"),
    rooms: [
      {
        room_number: "301",
        type: "Harbor View",
        price_per_night: NumberDecimal("280.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "302",
        type: "Harbor View",
        price_per_night: NumberDecimal("280.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-19T11:00:00Z")
      },
      {
        room_number: "201",
        type: "City View",
        price_per_night: NumberDecimal("200.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      }
    ]
  },
  {
    id: NumberLong(9),
    name: "Tech Campus Hotel",
    description: "Современный отель рядом с технологическими компаниями",
    city: "Seattle",
    address: "1100 Bellevue Way NE, Bellevue, WA 98004",
    stars: 4,
    created_by_user_id: NumberLong(5),
    created_at: new Date("2024-01-23T17:00:00Z"),
    updated_at: new Date("2024-01-23T17:00:00Z"),
    rooms: [
      {
        room_number: "501",
        type: "Smart Room",
        price_per_night: NumberDecimal("230.00"),
        capacity: 1,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "502",
        type: "Smart Room",
        price_per_night: NumberDecimal("230.00"),
        capacity: 1,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "601",
        type: "Tech Suite",
        price_per_night: NumberDecimal("400.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-24T11:00:00Z")
      }
    ]
  },
  {
    id: NumberLong(10),
    name: "Southern Charm B&B",
    description: "Уютный bed & breakfast с домашней атмосферой",
    city: "Nashville",
    address: "123 Music Row, Nashville, TN 37203",
    stars: 3,
    created_by_user_id: NumberLong(5),
    created_at: new Date("2024-01-24T18:00:00Z"),
    updated_at: new Date("2024-01-24T18:00:00Z"),
    rooms: [
      {
        room_number: "1",
        type: "Standard",
        price_per_night: NumberDecimal("90.00"),
        capacity: 2,
        is_available: true,
        checkout_date: null
      },
      {
        room_number: "2",
        type: "Standard",
        price_per_night: NumberDecimal("90.00"),
        capacity: 2,
        is_available: false,
        checkout_date: new Date("2024-02-17T11:00:00Z")
      },
      {
        room_number: "3",
        type: "Deluxe",
        price_per_night: NumberDecimal("140.00"),
        capacity: 3,
        is_available: true,
        checkout_date: null
      }
    ]
  }
]);

// 3. Вставка бронирований (минимум 10)
db.bookings.insertMany([
  {
    id: NumberLong(1),
    user_id: NumberLong(1),
    hotel_id: NumberLong(1),
    room_number: "102",
    check_in: new Date("2024-02-15T14:00:00Z"),
    check_out: new Date("2024-02-20T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("750.00"),
    created_at: new Date("2024-01-25T10:00:00Z"),
    updated_at: new Date("2024-01-25T10:00:00Z")
  },
  {
    id: NumberLong(2),
    user_id: NumberLong(2),
    hotel_id: NumberLong(2),
    room_number: "10B",
    check_in: new Date("2024-02-17T14:00:00Z"),
    check_out: new Date("2024-02-22T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("1500.00"),
    created_at: new Date("2024-01-26T11:00:00Z"),
    updated_at: new Date("2024-01-26T11:00:00Z")
  },
  {
    id: NumberLong(3),
    user_id: NumberLong(3),
    hotel_id: NumberLong(3),
    room_number: "5",
    check_in: new Date("2024-02-14T14:00:00Z"),
    check_out: new Date("2024-02-18T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("720.00"),
    created_at: new Date("2024-01-27T12:00:00Z"),
    updated_at: new Date("2024-01-27T12:00:00Z")
  },
  {
    id: NumberLong(4),
    user_id: NumberLong(4),
    hotel_id: NumberLong(4),
    room_number: "501",
    check_in: new Date("2024-02-20T14:00:00Z"),
    check_out: new Date("2024-02-25T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("1750.00"),
    created_at: new Date("2024-01-28T13:00:00Z"),
    updated_at: new Date("2024-01-28T13:00:00Z")
  },
  {
    id: NumberLong(5),
    user_id: NumberLong(5),
    hotel_id: NumberLong(5),
    room_number: "2B",
    check_in: new Date("2024-02-18T14:00:00Z"),
    check_out: new Date("2024-02-21T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("360.00"),
    created_at: new Date("2024-01-29T14:00:00Z"),
    updated_at: new Date("2024-01-29T14:00:00Z")
  },
  {
    id: NumberLong(6),
    user_id: NumberLong(6),
    hotel_id: NumberLong(6),
    room_number: "702",
    check_in: new Date("2024-02-25T14:00:00Z"),
    check_out: new Date("2024-02-28T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("1350.00"),
    created_at: new Date("2024-01-30T15:00:00Z"),
    updated_at: new Date("2024-01-30T15:00:00Z")
  },
  {
    id: NumberLong(7),
    user_id: NumberLong(7),
    hotel_id: NumberLong(7),
    room_number: "2001",
    check_in: new Date("2024-02-22T14:00:00Z"),
    check_out: new Date("2024-02-26T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("8000.00"),
    created_at: new Date("2024-01-31T16:00:00Z"),
    updated_at: new Date("2024-01-31T16:00:00Z")
  },
  {
    id: NumberLong(8),
    user_id: NumberLong(8),
    hotel_id: NumberLong(8),
    room_number: "302",
    check_in: new Date("2024-02-16T14:00:00Z"),
    check_out: new Date("2024-02-19T11:00:00Z"),
    status: "cancelled",
    total_price: NumberDecimal("840.00"),
    created_at: new Date("2024-02-01T17:00:00Z"),
    updated_at: new Date("2024-02-10T09:00:00Z")
  },
  {
    id: NumberLong(9),
    user_id: NumberLong(9),
    hotel_id: NumberLong(9),
    room_number: "601",
    check_in: new Date("2024-02-21T14:00:00Z"),
    check_out: new Date("2024-02-24T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("1200.00"),
    created_at: new Date("2024-02-02T18:00:00Z"),
    updated_at: new Date("2024-02-02T18:00:00Z")
  },
  {
    id: NumberLong(10),
    user_id: NumberLong(10),
    hotel_id: NumberLong(10),
    room_number: "2",
    check_in: new Date("2024-02-14T14:00:00Z"),
    check_out: new Date("2024-02-17T11:00:00Z"),
    status: "completed",
    total_price: NumberDecimal("270.00"),
    created_at: new Date("2024-02-03T19:00:00Z"),
    updated_at: new Date("2024-02-17T12:00:00Z")
  },
  {
    id: NumberLong(11),
    user_id: NumberLong(1),
    hotel_id: NumberLong(3),
    room_number: "1",
    check_in: new Date("2024-03-01T14:00:00Z"),
    check_out: new Date("2024-03-05T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("400.00"),
    created_at: new Date("2024-02-05T10:00:00Z"),
    updated_at: new Date("2024-02-05T10:00:00Z")
  },
  {
    id: NumberLong(12),
    user_id: NumberLong(2),
    hotel_id: NumberLong(7),
    room_number: "1501",
    check_in: new Date("2024-03-10T14:00:00Z"),
    check_out: new Date("2024-03-15T11:00:00Z"),
    status: "active",
    total_price: NumberDecimal("3000.00"),
    created_at: new Date("2024-02-06T11:00:00Z"),
    updated_at: new Date("2024-02-06T11:00:00Z")
  }
]);

print("Тестовые данные успешно загружены в MongoDB!");
print("Количество документов в коллекциях:");
print("users: " + db.users.count());
print("hotels: " + db.hotels.count());
print("bookings: " + db.bookings.count());
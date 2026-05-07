-- Clear data (for re-runs)
DELETE FROM bookings;
DELETE FROM sessions;
DELETE FROM hotels;
DELETE FROM users;

-- Reset sequences
ALTER SEQUENCE users_id_seq RESTART WITH 1;
ALTER SEQUENCE hotels_id_seq RESTART WITH 1;
ALTER SEQUENCE bookings_id_seq RESTART WITH 1;

-- =============================================================
-- Users (password = 'pass123' for all, hashed with fnv1a("password:pass123"))
-- hash value: 'a89e56a9b6e0d8c4' (precomputed)
-- =============================================================
INSERT INTO users (login, name, surname, email, password_hash) VALUES
('alice',      'Alice',     'Johnson',   'alice.j@example.com',    'a89e56a9b6e0d8c4'),
('bob',        'Bob',       'Williams',  'bob.w@example.com',      'a89e56a9b6e0d8c4'),
('charlie',    'Charlie',   'Brown',     'charlie.b@example.com',  'a89e56a9b6e0d8c4'),
('diana',      'Diana',     'Smith',     'diana.s@example.com',    'a89e56a9b6e0d8c4'),
('evan',       'Evan',      'Davis',     'evan.d@example.com',     'a89e56a9b6e0d8c4'),
('fiona',      'Fiona',     'Miller',    'fiona.m@example.com',    'a89e56a9b6e0d8c4'),
('george',     'George',    'Wilson',    'george.w@example.com',   'a89e56a9b6e0d8c4'),
('helen',      'Helen',     'Taylor',    'helen.t@example.com',    'a89e56a9b6e0d8c4'),
('ivan',       'Ivan',      'Anderson',  'ivan.a@example.com',     'a89e56a9b6e0d8c4'),
('julia',      'Julia',     'Thomas',    'julia.t@example.com',    'a89e56a9b6e0d8c4'),
('kate',       'Kate',      'Jackson',   'kate.j@example.com',     'a89e56a9b6e0d8c4'),
('leo',        'Leo',       'White',     'leo.w@example.com',      'a89e56a9b6e0d8c4');

-- =============================================================
-- Hotels (10+ with varied data)
-- =============================================================
INSERT INTO hotels (name, city, address, description, created_by_user_id, rating) VALUES
('Grand Plaza',       'Moscow',     'Tverskaya 1',    'Luxury 5-star hotel in the heart of Moscow',         1, 9.2),
('Seaside Resort',    'Sochi',      'Primorskaya 2',  'Beachfront resort with spa and pools',               1, 8.7),
('Alpine Lodge',      'Altay',      'Lesnaya 3',      'Cozy mountain lodge, ideal for skiing',              2, 8.1),
('City Comfort Inn',  'St. Petersburg', 'Nevsky 4',  'Affordable rooms near the Hermitage',                 3, 7.5),
('Sunset Motel',      'Sochi',      'Kurortnaya 5',   'Budget motel near the beach',                        4, 6.8),
('Taiga Hostel',      'Altay',      'Berezovaya 6',   'Backpacker-friendly hostel in nature',               5, 7.9),
('Business Hotel',    'Moscow',     'Leninsky 7',     'Modern hotel for business travelers',                6, 8.4),
('Volga Riviera',     'Volgograd',  'Naberezhnaya 8', 'Riverside hotel with panoramic views',               7, 8.9),
('Golden Sands',      'Anapa',      'Morskaya 9',     'Family resort on the Black Sea',                     8, 9.0),
('Northern Lights',   'Murmansk',   'Polyarnaya 10',  'Aurora viewing packages and sauna',                  9, 8.3),
('Siberian Hut',      'Irkutsk',    'Angarskaya 11',  'Traditional wooden cottages near Baikal',           10, 9.5),
('Caspian Pearl',     'Makhachkala','Kaspiyskaya 12', 'Luxury resort on the Caspian Sea',                  11, 8.6);

-- =============================================================
-- Bookings (mix of active, cancelled, future, past)
-- =============================================================
INSERT INTO bookings (user_id, hotel_id, check_in, check_out, status) VALUES
( 1,  1, '2026-07-10', '2026-07-15', 'active'),
( 2,  2, '2026-08-01', '2026-08-07', 'active'),
( 3,  3, '2026-09-20', '2026-09-25', 'active'),
( 4,  4, '2026-10-05', '2026-10-10', 'active'),
( 5,  5, '2026-06-15', '2026-06-20', 'cancelled'),
( 6,  6, '2026-12-01', '2026-12-05', 'active'),
( 7,  7, '2027-01-10', '2027-01-15', 'active'),
( 8,  8, '2027-02-14', '2027-02-18', 'active'),
( 9,  9, '2027-03-01', '2027-03-06', 'cancelled'),
(10, 10, '2027-04-22', '2027-04-27', 'active'),
(11, 11, '2027-05-10', '2027-05-14', 'active'),
(12, 12, '2027-06-01', '2027-06-07', 'active'),
( 1,  3, '2027-07-01', '2027-07-06', 'active'),   -- same user, different hotel
( 2,  1, '2027-08-10', '2027-08-15', 'cancelled');
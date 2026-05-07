-- Custom enum for booking status
CREATE TYPE booking_status AS ENUM ('active', 'cancelled');

-- =============================================================
-- Users
-- =============================================================
CREATE TABLE users (
    id            BIGSERIAL PRIMARY KEY,
    login         VARCHAR(100) NOT NULL,
    name          VARCHAR(255) NOT NULL,
    surname       VARCHAR(255) NOT NULL,
    email         VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at    TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    CONSTRAINT uq_users_login UNIQUE (login),
    CONSTRAINT chk_login_length CHECK (length(login) >= 3),
    CONSTRAINT chk_email CHECK (email LIKE '%_@_%.__%')
);

-- =============================================================
-- Sessions
-- =============================================================
CREATE TABLE sessions (
    token    VARCHAR(255) PRIMARY KEY,
    user_id  BIGINT       NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    login    VARCHAR(100) NOT NULL,
    created  TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

-- =============================================================
-- Hotels
-- =============================================================
CREATE TABLE hotels (
    id                 BIGSERIAL    PRIMARY KEY,
    name               VARCHAR(255) NOT NULL,
    city               VARCHAR(255) NOT NULL,
    address            VARCHAR(255) NOT NULL,
    description        TEXT,
    created_by_user_id BIGINT       NOT NULL REFERENCES users(id) ON DELETE RESTRICT,
    rating             NUMERIC(2,1) CHECK (rating BETWEEN 0 AND 10),
    created_at         TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

-- =============================================================
-- Bookings
-- =============================================================
CREATE TABLE bookings (
    id        BIGSERIAL PRIMARY KEY,
    user_id   BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    hotel_id  BIGINT NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    check_in  DATE   NOT NULL,
    check_out DATE   NOT NULL,
    status    booking_status NOT NULL DEFAULT 'active',
    created   TIMESTAMPTZ    NOT NULL DEFAULT NOW(),
    CONSTRAINT chk_dates CHECK (check_out > check_in)
);

-- =============================================================
-- Indexes (automatically created for PKs and UNIQUE constraints)
-- =============================================================

-- Foreign keys (many DBAs create them manually for performance)
CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_hotels_created_by ON hotels(created_by_user_id);
CREATE INDEX IF NOT EXISTS idx_bookings_user_id ON bookings(user_id);
CREATE INDEX IF NOT EXISTS idx_bookings_hotel_id ON bookings(hotel_id);

-- Frequently searched columns
CREATE INDEX IF NOT EXISTS idx_users_name_surname ON users(name, surname);
CREATE INDEX IF NOT EXISTS idx_users_login_lower ON users (LOWER(login));
CREATE INDEX IF NOT EXISTS idx_hotels_city_lower ON hotels (LOWER(city));
CREATE INDEX IF NOT EXISTS idx_hotels_rating ON hotels (rating DESC);

-- Index for looking up active bookings per user
CREATE INDEX IF NOT EXISTS idx_bookings_user_status ON bookings(user_id, status)
    WHERE status = 'active';

-- Index for finding overlapping bookings (useful if required later)
CREATE INDEX IF NOT EXISTS idx_bookings_dates ON bookings(hotel_id, check_in, check_out);

-- Index on login in sessions (for additional lookup)
CREATE INDEX IF NOT EXISTS idx_sessions_login ON sessions(login);
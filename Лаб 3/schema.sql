CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    login VARCHAR(50) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,  -- храним хеш пароля (например, bcrypt)
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    phone VARCHAR(20),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

COMMENT ON TABLE users IS 'Зарегистрированные пользователи системы';
COMMENT ON COLUMN users.id IS 'Уникальный идентификатор пользователя';
COMMENT ON COLUMN users.login IS 'Логин (уникальный)';
COMMENT ON COLUMN users.password_hash IS 'Хеш пароля';
COMMENT ON COLUMN users.first_name IS 'Имя';
COMMENT ON COLUMN users.last_name IS 'Фамилия';
COMMENT ON COLUMN users.email IS 'Email (уникальный)';
COMMENT ON COLUMN users.phone IS 'Номер телефона (опционально)';
COMMENT ON COLUMN users.created_at IS 'Дата и время создания записи';
COMMENT ON COLUMN users.updated_at IS 'Дата и время последнего обновления';

CREATE TABLE hotels (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(200) NOT NULL,
    description TEXT,
    city VARCHAR(100) NOT NULL,
    address VARCHAR(255) NOT NULL,
    stars SMALLINT CHECK (stars >= 1 AND stars <= 5),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

COMMENT ON TABLE hotels IS 'Информация об отелях';
COMMENT ON COLUMN hotels.id IS 'Уникальный идентификатор отеля';
COMMENT ON COLUMN hotels.name IS 'Название отеля';
COMMENT ON COLUMN hotels.description IS 'Описание отеля';
COMMENT ON COLUMN hotels.city IS 'Город расположения';
COMMENT ON COLUMN hotels.address IS 'Полный адрес';
COMMENT ON COLUMN hotels.stars IS 'Количество звезд (1-5)';
COMMENT ON COLUMN hotels.created_at IS 'Дата и время добавления';
COMMENT ON COLUMN hotels.updated_at IS 'Дата и время обновления';

CREATE TABLE bookings (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    hotel_id BIGINT NOT NULL,
    check_in_date DATE NOT NULL,
    check_out_date DATE NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'active',
    total_price DECIMAL(10,2) NOT NULL CHECK (total_price >= 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    
    CONSTRAINT fk_bookings_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_bookings_hotel FOREIGN KEY (hotel_id) REFERENCES hotels(id) ON DELETE CASCADE,
    CONSTRAINT chk_dates CHECK (check_out_date > check_in_date),
    CONSTRAINT chk_status CHECK (status IN ('active', 'cancelled', 'completed'))
);

COMMENT ON TABLE bookings IS 'Бронирования отелей пользователями';
COMMENT ON COLUMN bookings.id IS 'Уникальный идентификатор бронирования';
COMMENT ON COLUMN bookings.user_id IS 'ID пользователя, сделавшего бронь';
COMMENT ON COLUMN bookings.hotel_id IS 'ID забронированного отеля';
COMMENT ON COLUMN bookings.check_in_date IS 'Дата заезда';
COMMENT ON COLUMN bookings.check_out_date IS 'Дата выезда';
COMMENT ON COLUMN bookings.status IS 'Статус бронирования: active, cancelled, completed';
COMMENT ON COLUMN bookings.total_price IS 'Общая стоимость бронирования';
COMMENT ON COLUMN bookings.created_at IS 'Дата создания брони';
COMMENT ON COLUMN bookings.updated_at IS 'Дата последнего изменения';

CREATE INDEX idx_users_first_name_pattern ON users(first_name text_pattern_ops);
CREATE INDEX idx_users_last_name_pattern ON users(last_name text_pattern_ops);
CREATE INDEX idx_users_first_last ON users(first_name, last_name);

CREATE INDEX idx_hotels_city ON hotels(city);
CREATE INDEX idx_hotels_name ON hotels(name);
CREATE INDEX idx_hotels_stars ON hotels(stars);

CREATE INDEX idx_bookings_user_id ON bookings(user_id);
CREATE INDEX idx_bookings_hotel_id ON bookings(hotel_id);
CREATE INDEX idx_bookings_user_status ON bookings(user_id, status);
CREATE INDEX idx_bookings_dates ON bookings(check_in_date, check_out_date);
CREATE INDEX idx_bookings_status ON bookings(status);


CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER trg_hotels_updated_at
    BEFORE UPDATE ON hotels
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER trg_bookings_updated_at
    BEFORE UPDATE ON bookings
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();
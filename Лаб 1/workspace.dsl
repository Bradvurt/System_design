workspace "Hotel Booking System" "Система бронирования отелей – вариант 13" {

    model {

        # Пользователи
        guest = person "Гость" "Неавторизованный пользователь, ищет отели"
        registeredUser = person "Зарегистрированный пользователь" "Клиент, управляет своими бронированиями"
        hotelManager = person "Менеджер отеля" "Управляет своим отелем"
        admin = person "Администратор сервиса" "Полные права на управление системой"

        # Внешние системы
        emailService = softwareSystem "Email-сервис" "Отправляет email-уведомления" {
            tags "external"
        }
        paymentSystem = softwareSystem "Платежная система" "Внешний шлюз для обработки платежей" {
            tags "external"
        }
        mapService = softwareSystem "Картографический сервис" "Google Maps для карт и геокодирования" {
            tags "external"
        }

        # Система с контейнерами
        hotelSystem = softwareSystem "Система бронирования отелей" "Позволяет искать и бронировать отели" {
            
            # Базы данных
            userDb = container "User Database" "Хранит данные пользователей" "PostgreSQL" {
                tags "database"
            }
            hotelDb = container "Hotel Database" "Хранит данные об отелях" "PostgreSQL" {
                tags "database"
            }
            bookingDb = container "Booking Database" "Хранит данные о бронированиях" "PostgreSQL" {
                tags "database"
            }
            paymentDb = container "Payment Database" "Хранит данные о транзакциях" "PostgreSQL" {
                tags "database"
            }
            
            # Микросервисы
            notificationService = container "Notification Service" "Отправка уведомлений по email" "Spring Boot" {
                -> emailService "Отправляет email-уведомления" "SMTP/HTTPS"
            }
            messageBroker = container "Message Broker" "Асинхронная шина событий" "RabbitMQ / Kafka" {
                -> notificationService "Доставляет события для отправки уведомлений" "AMQP/Kafka"
                tags "message-broker"
            }
            
            userService = container "User Service" "Управление пользователями: регистрация, поиск" "Spring Boot" {
                 -> userDb "Читает/пишет" "JDBC"
                 -> messageBroker "Публикует события (UserRegistered, UserUpdated)" "AMQP/Kafka"
            }
            
            bookingService = container "Booking Service" "Управление бронированиями: создание, отмена, проверка доступности" "Spring Boot" {
                -> messageBroker "Публикует события (BookingCreated, BookingCancelled)" "AMQP/Kafka"
                -> bookingDb "Читает/пишет" "JDBC"
            }
            
            hotelService = container "Hotel Service" "Управление отелями: создание, поиск по городу, интеграция с картами" "Spring Boot" {
                -> hotelDb "Читает/пишет" "JDBC"
                -> messageBroker "Публикует события (HotelCreated, HotelUpdated)" "AMQP/Kafka"
                -> bookingService "Уведомляет о новом отеле/номерах для возможности бронирования" "HTTPS/REST"
                -> mapService "Геокодирование, отображение карт" "HTTPS/REST"
            }
            
            paymentService = container "Payment Service" "Обработка платежей, интеграция с внешней платежной системой" "Spring Boot" {
                -> paymentDb "Читает/пишет" "JDBC"
                -> paymentSystem "Проводит платежи" "HTTPS/REST"
            }
            
            # API Gateway
            apiGateway = container "API Gateway" "Единая точка входа, маршрутизация, аутентификация" "Spring Cloud Gateway" {
                -> userService "Маршрутизирует запросы /api/users/**" "HTTPS/REST"
                -> hotelService "Маршрутизирует запросы /api/hotels/**" "HTTPS/REST"
                -> bookingService "Маршрутизирует запросы /api/bookings/**" "HTTPS/REST"
            }
            
            # Веб-приложение
            spa = container "Single-Page Application" "Веб-интерфейс для пользователей" "React" {
                -> apiGateway "Использует" "HTTPS/REST"
            }
            
            # Связи пользователей
            guest -> spa "Ищет отели, просматривает"
            registeredUser -> spa "Управляет бронированиями"
            hotelManager -> spa "Управляет своим отелем"
            admin -> spa "Администрирует систему"

            # Синхронные вызовы между сервисами
            bookingService -> paymentService "Инициирует платеж, проверяет статус" "HTTPS/REST"
        }
    }

    views {
        
        themes default
        
        # Диаграмма контекста
        systemContext hotelSystem "SystemContext" "Диаграмма контекста системы бронирования отелей" {
            include *
            autolayout lr
        }
    
        # Контейнерная диаграмма
        container hotelSystem "Container" "Диаграмма контейнеров системы бронирования отелей" {
            include *
            autolayout lr
        }
        
        # Динамическая диаграмма
        dynamic hotelSystem "CreateBooking" "Динамическая диаграмма процесса создания бронирования" {
            
            1: registeredUser -> spa "Заполняет форму бронирования и нажимает кнопку Забронировать"
            2: spa -> apiGateway "POST /api/bookings (отель, даты, данные пользователя)"
            3: apiGateway -> bookingService "POST /bookings (те же данные)"
            4: bookingService -> bookingDb "Проверяет наличие свободных номеров"
            5: bookingDb -> bookingService "Возвращает результат (доступно/недоступно, цена)"
            6: bookingService -> bookingDb "Создает запись бронирования со статусом Зарезервированно"
            7: bookingDb -> bookingService "Подтверждение создания"
            8: bookingService -> paymentService "POST /payments (сумма, детали брони)"
            9: paymentService -> paymentDb "Создает запись транзакции со статусом Ожидает оплаты"
            10: paymentDb -> paymentService "Подтверждение создания"
            11: paymentService -> paymentSystem "Запрос на обработку платежа"
            12: paymentSystem -> paymentService "Возвращает результат (успех/неудача)"
            13: paymentService -> paymentDb "Обновляет статус транзакции"
            14: paymentDb -> paymentService "Подтверждение обновления"
            15: paymentService -> bookingService "Возвращает результат платежа"
            16: bookingService -> bookingDb "Обновляет статус бронирования на Забронировано (или Отменено при неудаче)"
            17: bookingDb -> bookingService "Подтверждение обновления"
            18: bookingService -> messageBroker "Публикует событие BookingCreated (с деталями)"
            19: messageBroker -> notificationService "Доставляет событие"
            20: notificationService -> emailService "Отправляет email-подтверждение пользователю"
            21: bookingService -> apiGateway "Возвращает успешный ответ с данными бронирования"
            22: apiGateway -> spa "Возвращает ответ"
            23: spa -> registeredUser "Отображает страницу подтверждения"

            autolayout
        }

        styles {
             element "database" {
                shape cylinder
                background #554DB8
                color #ffffff
                fontSize 12
            }
            element "message-broker" {
                shape folder
                background #438dd5
                color #ffffff
                fontSize 12
            }
        }
    }
}
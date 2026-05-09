cmake_minimum_required(VERSION 3.16)
project(hotel_booking_lab5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(userver COMPONENTS core redis REQUIRED)

add_library(hotel_booking_core STATIC
    src/models.cpp
    src/auth.cpp
    src/cache.cpp
    src/rate_limiter.cpp
)

target_include_directories(hotel_booking_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(hotel_booking_core PUBLIC
    userver::core
    userver::redis
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/handlers.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    hotel_booking_core
    userver::core
    userver::redis
)
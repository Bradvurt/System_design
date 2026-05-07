cmake_minimum_required(VERSION 3.10)
project(hotel_booking LANGUAGES CXX)

# Add our dummy modules path before calling find_package(userver)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Disable all userver features we don't need
set(USERVER_FEATURE_ODBC OFF CACHE BOOL "")
set(USERVER_FEATURE_POSTGRESQL OFF CACHE BOOL "")
set(USERVER_FEATURE_CLICKHOUSE OFF CACHE BOOL "")
set(USERVER_FEATURE_REDIS OFF CACHE BOOL "")
set(USERVER_FEATURE_MONGODB OFF CACHE BOOL "")
set(USERVER_FEATURE_GRPC OFF CACHE BOOL "")

find_package(userver REQUIRED)

add_executable(hotel_booking
    src/main.cpp
    src/models.cpp
    src/handlers.cpp
    src/auth.cpp
)

target_link_libraries(hotel_booking
    userver::core
    userver::server
    userver::http
    userver::formats
    userver::crypto
)
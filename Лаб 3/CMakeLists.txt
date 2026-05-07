cmake_minimum_required(VERSION 3.14)
project(hotel_booking_lab3)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(USERVER_FEATURE_ODBC OFF)

find_package(userver REQUIRED COMPONENTS core postgresql)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/handlers.cpp
    src/models.cpp
    src/auth.cpp
)

target_link_libraries(${PROJECT_NAME}
    userver::core
    userver::postgresql
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

file(COPY config/static_config.yaml DESTINATION ${CMAKE_BINARY_DIR}/)

# --- Unit tests (same as original lab3) ---
add_executable(${PROJECT_NAME}_service_tests
    tests/service_test.cpp
    src/models.cpp
    src/auth.cpp
)

target_link_libraries(${PROJECT_NAME}_service_tests
    userver::core
    userver::postgresql
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(${PROJECT_NAME}_service_tests PRIVATE src)

add_test(NAME hotel_booking_lab3_service_tests COMMAND ${PROJECT_NAME}_service_tests)
cmake_minimum_required(VERSION 3.14)
project(hotel_booking)

include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(USERVER_FEATURE_ODBC OFF)

find_package(userver REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(hotel_booking
    src/main.cpp
    src/handlers.cpp
    src/models.cpp
    src/auth.cpp
)

target_link_libraries(hotel_booking
    userver::core
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(hotel_booking PRIVATE
    src
)

file(COPY config/config.yaml DESTINATION ${CMAKE_BINARY_DIR}/)
file(COPY openapi.yaml DESTINATION ${CMAKE_BINARY_DIR}/)

if(BUILD_TESTING)
    add_executable(unit_tests
        tests/unit_tests.cpp
        src/models.cpp
        src/auth.cpp
    )
    target_link_libraries(unit_tests
        userver::core
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    target_include_directories(unit_tests PRIVATE
        src
    )
    add_test(NAME unit_tests COMMAND unit_tests)
endif()

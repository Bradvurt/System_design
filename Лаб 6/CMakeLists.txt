cmake_minimum_required(VERSION 3.16)
project(hotel_booking_lab6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(userver COMPONENTS core mongo rabbitmq REQUIRED)

# Domain library
add_library(hotel_booking_core STATIC
    src/domain/hotel_booking_service.cpp
)
target_include_directories(hotel_booking_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include/libbson-1.0
    /usr/include/libmongoc-1.0
)
target_link_libraries(hotel_booking_core PUBLIC
    userver::core
    userver::mongo
)

# HTTP layer (handlers and middleware)
add_library(hotel_booking_core OBJECT
    src/api/api_utils.cpp
    src/api/middleware/auth_middleware.cpp
    src/api/handlers/auth_handlers.cpp
    src/api/handlers/user_handlers.cpp
    src/api/handlers/hotel_handlers.cpp
    src/api/handlers/booking_handlers.cpp
)
target_include_directories(hotel_booking_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    /usr/include/libbson-1.0
    /usr/include/libmongoc-1.0
)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    $<TARGET_OBJECTS:hotel_booking_core>
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    hotel_booking_core
    userver::core
    userver::mongo
    userver::urabbitmq
)

# Optional: Tests
# add_executable(${PROJECT_NAME}_service_tests tests/service_test.cpp)
# target_link_libraries(${PROJECT_NAME}_service_tests PRIVATE hotel_booking_core)
# enable_testing()
# add_test(NAME hotel_booking_lab6_service_tests COMMAND ${PROJECT_NAME}_service_tests)
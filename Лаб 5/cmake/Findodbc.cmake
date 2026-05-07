# Dummy Findodbc.cmake – satisfies userver's internal find_package(odbc)
# without actually needing the ODBC library (the feature is disabled).
if(NOT TARGET odbc::odbc)
    add_library(odbc::odbc INTERFACE IMPORTED)
endif()
set(odbc_FOUND TRUE)
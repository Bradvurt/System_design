include(FindPackageHandleStandardArgs)

find_path(ODBC_INCLUDE_DIR
    NAMES sql.h
    PATH_SUFFIXES unixodbc
)

find_library(ODBC_LIBRARY
    NAMES odbc iodbc odbc32
)

set(ODBC_INCLUDE_DIRS "${ODBC_INCLUDE_DIR}")
set(ODBC_LIBRARIES "${ODBC_LIBRARY}")
set(odbc_INCLUDE_DIRS "${ODBC_INCLUDE_DIR}")
set(odbc_LIBRARIES "${ODBC_LIBRARY}")

find_package_handle_standard_args(odbc DEFAULT_MSG ODBC_LIBRARY ODBC_INCLUDE_DIR)

if(odbc_FOUND AND NOT TARGET odbc)
    add_library(odbc INTERFACE IMPORTED)
    set_target_properties(odbc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ODBC_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ODBC_LIBRARY}"
    )
endif()

if(odbc_FOUND AND NOT TARGET odbc::odbc)
    add_library(odbc::odbc INTERFACE IMPORTED)
    set_target_properties(odbc::odbc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ODBC_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ODBC_LIBRARY}"
    )
endif()

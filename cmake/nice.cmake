
include(FindPkgConfig)
pkg_check_modules(LIBNICE nice>=0.1.16)

if (LIBNICE_FOUND)
    message(STATUS "Found nice v${LIBNICE_VERSION}")
    message(VERBOSE "LIBNICE_INCLUDEDIR=${LIBNICE_INCLUDEDIR}")
    message(VERBOSE "LIBNICE_LIBRARY_DIRS=${LIBNICE_LIBRARY_DIRS}")
    message(VERBOSE "LIBNICE_LINK_LIBRARIES=${LIBNICE_LINK_LIBRARIES}")
    message(VERBOSE "LIBNICE_LIBRARIES=${LIBNICE_LIBRARIES}")
else()
    message(SEND_ERROR "-- Could not find libnice")
endif()

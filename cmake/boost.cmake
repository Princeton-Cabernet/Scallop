
find_package(Boost 1.74 CONFIG REQUIRED)

if (Boost_FOUND)
    message(STATUS "Found boost v${Boost_VERSION}")
    message(VERBOSE "Boost_INCLUDEDIR=${Boost_INCLUDEDIR}")
    message(VERBOSE "Boost_LIBRARIES=${Boost_LIBRARIES}")
else()
    message(SEND_ERROR "Could not find boost")
endif()

set(CATCH2_VERSION 2.13.10)

if (NOT EXISTS ${CMAKE_HOME_DIRECTORY}/test/include/catch.h)
    file(DOWNLOAD
            https://github.com/catchorg/Catch2/releases/download/v${CATCH2_VERSION}/catch.hpp
            ${CMAKE_HOME_DIRECTORY}/test/include/catch.h)
    message(STATUS "Downloading Catch2: /test/include/catch.h - done")
endif ()

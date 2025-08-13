
set(INJA_VERSION 3.4.0)

if(NOT EXISTS ${CMAKE_HOME_DIRECTORY}/ext/include/inja/inja.hpp)
    file(DOWNLOAD
            https://github.com/pantor/inja/releases/download/v${INJA_VERSION}/inja.hpp
            ${CMAKE_HOME_DIRECTORY}/ext/include/inja/inja.hpp)
    message(STATUS "Downloading inja: /ext/include/inja/inja.hpp - done")
endif()




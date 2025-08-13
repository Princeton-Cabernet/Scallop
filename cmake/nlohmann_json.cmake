
set(NLOHMANN_JSON_VERSION 3.11.3)

if(NOT EXISTS ${CMAKE_HOME_DIRECTORY}/ext/include/nlohmann/json.hpp)
    file(DOWNLOAD
            https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/json.hpp
            ${CMAKE_HOME_DIRECTORY}/ext/include/nlohmann/json.hpp)
    message(STATUS "Downloading nlohmann_json: /ext/include/nlohmann/json.hpp - done")
endif()


cmake_minimum_required(VERSION 3.22)
project(scallop VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    # use old linker strategy due to incompatibility of Xcode 15 with GCC13 on macOS
    # remove once fixed:
    set(CMAKE_CXX_FLAGS "-Wl,-ld_classic")
endif()

cmake_policy(SET CMP0167 NEW)

include(cmake/boost.cmake)
include(cmake/catch2.cmake)
include(cmake/cxxopts.cmake)
include(cmake/inja.cmake)
include(cmake/nice.cmake)
include(cmake/nlohmann_json.cmake)
include(cmake/pcap.cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib)


add_subdirectory(test)


# if (NOT DEFINED WITH_CONTROLLER
#     AND NOT DEFINED WITH_MODEL
#     AND NOT DEFINED WITH_TOFINO_AGENT
#     AND NOT DEFINED WITH_UTIL)
#     set(BUILD_EVERYTHING ON)
# endif()

# if ((DEFINED WITH_CONTROLLER AND WITH_CONTROLLER) OR BUILD_EVERYTHING)
#     add_subdirectory(controller)
# endif()

# if ((DEFINED WITH_MODEL AND WITH_MODEL) OR BUILD_EVERYTHING)
#     add_subdirectory(model)
# endif()

# if ((DEFINED WITH_TOFINO_AGENT AND WITH_TOFINO_AGENT) OR BUILD_EVERYTHING)
#     add_subdirectory(tofino_agent)
# endif()

# if ((DEFINED WITH_UTIL AND WITH_UTIL) OR BUILD_EVERYTHING)
#     add_subdirectory(util)
# endif()

# if (BUILD_EVERYTHING)
#     add_subdirectory(test)
#     add_subdirectory(eval/pkt_types)
#     add_subdirectory(eval/parse_rtp)
# endif()

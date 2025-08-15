
set(CONTROLLER_CLI_LIB_FILES
        api_client.h api_client.cc
        net/web_socket_client.h
        controller_api_client.h controller_api_client.cc
        util.h)

list(TRANSFORM CONTROLLER_CLI_LIB_FILES PREPEND ${LIB_DIR}/)

add_executable(controller_cli controller_cli_main.cc ${CONTROLLER_CLI_LIB_FILES})

target_include_directories(controller_cli PRIVATE ../ext/include)
target_include_directories(controller_cli PRIVATE  ${Boost_INCLUDE_DIRS})
target_link_libraries(controller_cli PRIVATE ${Boost_LIBRARIES})
target_link_libraries(controller_cli PRIVATE Threads::Threads)
set_target_properties(controller_cli PROPERTIES LINKER_LANGUAGE CXX)


set(SWITCH_CLI_LIB_FILES
        api_client.h api_client.cc
        net/web_socket_client.h
        switch_api_client.h switch_api_client.cc
        util.h)

list(TRANSFORM SWITCH_CLI_LIB_FILES PREPEND ${LIB_DIR}/)

add_executable(switch_cli switch_cli_main.cc ${SWITCH_CLI_LIB_FILES})

target_include_directories(switch_cli PRIVATE ../ext/include)
target_include_directories(switch_cli PRIVATE  ${Boost_INCLUDE_DIRS})
target_link_libraries(switch_cli PRIVATE ${Boost_LIBRARIES})
target_link_libraries(switch_cli PRIVATE Threads::Threads)
set_target_properties(switch_cli PROPERTIES LINKER_LANGUAGE CXX)

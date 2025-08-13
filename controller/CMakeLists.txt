
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)

if(OPENSSL_FOUND)
    message(STATUS "Detecting libopenssl - done
   OPENSSL_INCLUDE_DIRS: ${OPENSSL_INCLUDE_DIRS}
   OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}
   OPENSSL_LINK_LIBRARIES: ${OPENSSL_LINK_LIBRARIES}
   OPENSSL_VERSION: ${OPENSSL_VERSION}")
endif()

set(CONTROLLER_LIB_FILES
    api.h
    bitstream.h bitstream.cc
    controller_api.h controller_api.cc
    controller_client_connection.h controller_client_connection.cc
    controller_client_interface.h controller_client_interface.cc
    controller_switch_connection.h controller_switch_connection.cc
    controller_switch_interface.h controller_switch_interface.cc
    controller_web_server.h controller_web_server.cc
    log.h log.cc
    net/http_server.h
    net/net.h
    net/secure_web_socket_server.h
    net/secure_web_socket_session.h
    net/ssl.h
    net/tcp_server.h
    net/tcp_session.h
    net/web_socket_server.h
    net/web_socket_session.h
    participant.h participant.cc
    proto/sdp.h proto/sdp.cc
    rpc.h rpc.cc
    session.h session.cc
    session_manager.h session_manager.cc
    signaling_message.h
    stream.h stream.cc
    timer.h)

list(TRANSFORM CONTROLLER_LIB_FILES PREPEND ${LIB_DIR}/)

add_executable(controller
    controller_main.cc
    controller.h controller.cc
    ${CONTROLLER_LIB_FILES})

target_include_directories(controller PRIVATE ../ext/include)
target_include_directories(controller PRIVATE  ${Boost_INCLUDE_DIRS})
target_include_directories(controller PRIVATE ${OPENSSL_INCLUDE_DIRS})
target_link_libraries(controller PRIVATE ${Boost_LIBRARIES})
target_link_libraries(controller PRIVATE Threads::Threads)
target_link_libraries(controller PRIVATE ${OPENSSL_LINK_LIBRARIES})
set_target_properties(controller PROPERTIES LINKER_LANGUAGE CXX)

# add_executable(wss_test
#         wss_test_main.cc
#         ${CONTROLLER_LIB_FILES})

# target_include_directories(wss_test PRIVATE ../ext/include)
# target_include_directories(wss_test PRIVATE  ${Boost_INCLUDE_DIRS})
# target_include_directories(wss_test PRIVATE ${OPENSSL_INCLUDE_DIRS})
# target_link_libraries(wss_test PRIVATE ${Boost_LIBRARIES})
# target_link_libraries(wss_test PRIVATE Threads::Threads)
# target_link_libraries(wss_test PRIVATE ${OPENSSL_LINK_LIBRARIES})
# set_target_properties(wss_test PROPERTIES LINKER_LANGUAGE CXX)

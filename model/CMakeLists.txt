
set(MODEL_LIB_FILES
    av1.h av1.cc
    api.h
    data_plane.h
    data_plane_model.h data_plane_model.cc
    drop_layer_set.h
    log.h log.cc
    net/net.h
    net/tcp_client.h
    net/udp_server.h
    p4sfu.h
    proto/rtcp.h
    proto/rtp.h
    proto/sdp.h proto/sdp.cc
    proto/stun.h
    rpc.h rpc.cc
    sequence_rewriter.h sequence_rewriter.cc
    sfu_table.h sfu_table.cc
    stun_agent.h stun_agent.cc
    switch_agent.h
    switch_agent_state.h
    switch_controller_client.h switch_controller_client.cc
    switch_statistics.h
    switch_api.h switch_api.cc)

list(TRANSFORM MODEL_LIB_FILES PREPEND ${LIB_DIR}/)

add_executable(model model_main.cc ${MODEL_LIB_FILES})

target_include_directories(model PRIVATE ../ext/include)
target_include_directories(model PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(model PRIVATE ${LIBNICE_INCLUDEDIR})
target_link_libraries(model PUBLIC ${Boost_LIBRARIES})
target_link_libraries(model PUBLIC ${LIBNICE_LINK_LIBRARIES})
target_link_libraries(model PUBLIC Threads::Threads)
set_target_properties(model PROPERTIES LINKER_LANGUAGE CXX)

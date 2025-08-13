
set(LIB_FILES
    av1.h av1.cc
    bitstream.h bitstream.cc
    data_plane_model.h data_plane_model.cc
    drop_layer_set.h
    log.h log.cc
    net/net.h
    participant.h participant.cc
    proto/sdp.h proto/sdp.cc
    proto/stun.h
    rpc.h rpc.cc
    sequence_rewriter.h sequence_rewriter.cc
    session.h session.cc
    session_manager.h session_manager.cc
    sfu_config.h
    sfu_table.h sfu_table.cc
    signaling_message.h
    stream.h stream.cc
    stun_agent.h stun_agent.cc
    switch_agent_state.h
    util.h)

list(TRANSFORM LIB_FILES PREPEND ${LIB_DIR}/)

set(TEST_UNIT_FILES
    av1_test.cc
    bitstream_test.cc
    data_plane_model_test.cc
    drop_layer_set_test.cc
    libnice_test.cc
    misc_data.h
    mock/mock_data_plane.h
    multi_peer_conn_signaling_test.cc
    net_test.cc
    participant_test.cc
    rpc_messages.h
    rpc_test.cc
    rtcp_test.cc
    rtp_test.cc
    sdp_test.cc
    sequence_rewriter_test.cc
    session_manager_test.cc
    session_test.cc
    sfu_config_test.cc
    sfu_table_test.cc
    stream_test.cc
    stun_agent_test.cc
    stun_packets.h
    stun_test.cc
    switch_agent_state_test.cc
    util_test.cc)

add_executable(unit unit_main.cc
        ${TEST_UNIT_FILES}
        ${LIB_FILES})

target_include_directories(unit PRIVATE ../ext/include)
target_include_directories(unit PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(unit PRIVATE include)
target_include_directories(unit PRIVATE ${LIBNICE_INCLUDEDIR})
target_include_directories(unit PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(unit PRIVATE ${LIBNICE_LINK_LIBRARIES})
target_link_libraries(unit PUBLIC ${Boost_LIBRARIES})
target_link_libraries(unit PUBLIC Threads::Threads)
set_target_properties(unit PROPERTIES LINKER_LANGUAGE CXX)

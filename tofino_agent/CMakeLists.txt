
set(TOFINO_AGENT_LIB_FILES
        av1.h av1.cc
        data_plane.h
        file_descriptor.h
        log.h log.cc
        net/net.h
        net/pcap_interface.h
        net/tcp_client.h
        net/udp_server.h
        proto/rtcp.h
        proto/rtp.h
        proto/sdp.h proto/sdp.cc
        proto/stun.h
        rpc.h rpc.cc
        sfu_table.h sfu_table.cc
        stun_agent.h stun_agent.cc
        switch_agent.h
        switch_controller_client.h switch_controller_client.cc
        switch_statistics.h
        tofino_data_plane.h tofino_data_plane.cc
        switch_api.h switch_api.cc)

list(TRANSFORM TOFINO_AGENT_LIB_FILES PREPEND ${LIB_DIR}/)

add_executable(tofino_agent tofino_agent_main.cc ${TOFINO_AGENT_LIB_FILES})

target_include_directories(tofino_agent PRIVATE ../ext/include)
target_include_directories(tofino_agent PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(tofino_agent PRIVATE ${LIBNICE_INCLUDEDIR})
target_include_directories(tofino_agent PRIVATE ${PCAP_INCLUDE_DIRS})
target_link_libraries(tofino_agent PUBLIC ${Boost_LIBRARIES})
target_link_libraries(tofino_agent PUBLIC ${LIBNICE_LINK_LIBRARIES})
target_link_libraries(tofino_agent PUBLIC Threads::Threads)
target_link_libraries(tofino_agent PUBLIC ${PCAP_LIBRARIES})
set_target_properties(tofino_agent PROPERTIES LINKER_LANGUAGE CXX)


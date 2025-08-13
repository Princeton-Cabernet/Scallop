#include <catch.h>

#include <stun_agent.h>

#include "stun_packets.h"

using namespace p4sfu;

#include <iomanip>

TEST_CASE("STUNAgent", "[stun_agent]") {

    StunAgent stun;

    auto flags = static_cast<StunAgentUsageFlags>(
        STUN_AGENT_USAGE_SHORT_TERM_CREDENTIALS | STUN_AGENT_USAGE_USE_FINGERPRINT
    );

    stun_agent_init(&stun, STUN_ALL_KNOWN_ATTRIBUTES, STUN_COMPATIBILITY_RFC5389, flags);

    // -- validate binding request

    StunMessage bindRequest;

    StunDefaultValidaterData validaterData = {
            test::stun_bind_request_ice_ufrag, 9,
            test::stun_bind_request_ice_pwd, 24
    };

    auto valRes = stun_agent_validate(&stun,
                                      &bindRequest,
                                      (const unsigned char*) test::stun_bind_req,
                                      test::stun_bind_req_len,
                                      stun_agent_default_validater,
                                      &validaterData);

    CHECK(valRes == STUN_VALIDATION_SUCCESS);

    // -- create binding response

    StunMessage bindResponse;
    std::size_t bindResponseBufLen = 512;
    unsigned char bindResponseBuf[512] = {0};

    bool iceControlling = false;

    if (stun_message_has_attribute(&bindRequest, STUN_ATTRIBUTE_ICE_CONTROLLED)) {
        iceControlling = true;
    } else if (stun_message_has_attribute(&bindRequest, STUN_ATTRIBUTE_ICE_CONTROLLING)) {
        iceControlling = false;
    }

    // address where stun binding request came from:
    struct sockaddr_in from_sockaddr{};
    from_sockaddr.sin_family = AF_INET;
    from_sockaddr.sin_addr.s_addr = htonl(net::IPv4{"1.2.3.4"}.num());
    from_sockaddr.sin_port = htons(1234);

    auto result = stun_usage_ice_conncheck_create_reply(&stun,
                                                        &bindRequest,
                                                        &bindResponse,
                                                        bindResponseBuf,
                                                        &bindResponseBufLen,
                                                        (const sockaddr_storage*) &from_sockaddr,
                                                        sizeof(from_sockaddr),
                                                        &iceControlling,
                                                        0,
                                                        STUN_USAGE_ICE_COMPATIBILITY_RFC5245);

    CHECK(result == STUN_USAGE_ICE_RETURN_SUCCESS);
    CHECK(bindResponseBufLen == 80);

//    for (auto i = 0; i < 80; i++) {
//        std::cout << std::setw(2) << std::setfill('0') << std::hex << (unsigned) bindResponseBuf[i] << " ";
//        if (i % 8 == 7) std::cout << std::endl;
//    }
}

TEST_CASE("STUNAgent: addPeer", "[stun_agent]") {

    STUNAgent::Config config{.credentials = {"Q9uT", "Y00rHaGahe7tHgvA/iqU9wJI"}};
    STUNAgent a{config};

    net::IPv4Port from{net::IPv4{"12.12.12.42"}, 23212};

    SECTION("add a new peer succeeds") {
        CHECK_NOTHROW(a.addPeer(from, {"Q9uT", "UKZe/aYNEouGzQUhChnKGiIS"}));
        CHECK(a.countPeers() == 1);
    }

    SECTION("add an already existing peer fails") {
        CHECK_NOTHROW(a.addPeer(from, {"Q9uT", "UKZe/aYNEouGzQUhChnKGiIS"}));
        CHECK_THROWS(a.addPeer(from, {"Q9uT", "UKZe/aYNEouGzQUhChnKGiIS"}));
        CHECK(a.countPeers() == 1);
    }
}

TEST_CASE("STUNAgent: processBindRequest", "[stun_agent]") {

    STUNAgent::Config config{.credentials = {"Q9uT", "Y00rHaGahe7tHgvA/iqU9wJI"}};
    STUNAgent a{config};

    net::IPv4Port from{net::IPv4{"12.12.12.42"}, 23212};
    a.addPeer(from, {"Q9uT", "UKZe/aYNEouGzQUhChnKGiIS"});

    CHECK(a.countPeers() == 1);
    CHECK(a.validate(from, test::stun_bind_req, test::stun_bind_req_len).success);

    CHECK(a.msgLen() == 80);

    auto* response = a.msgBuf();

    // binding response success:
    CHECK(response[0] == 0x01);
    CHECK(response[1] == 0x01);

    // magic cookie:
    CHECK(response[4] == 0x21);
    CHECK(response[5] == 0x12);
    CHECK(response[6] == 0xa4);
    CHECK(response[7] == 0x42);
}

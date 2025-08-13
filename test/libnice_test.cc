
#include <catch.h>
#include "stun_packets.h"

extern "C" {
#include <stun/stunagent.h>
#include <stun/usages/ice.h>
}

#include <arpa/inet.h>

TEST_CASE("validate and respond to stun binding request", "[nice]") {

    // initialize STUN agent:

    StunAgent stunAgent;
    StunMessage stunMsg;

    auto agentUsageFlags = static_cast<StunAgentUsageFlags>(
        STUN_AGENT_USAGE_SHORT_TERM_CREDENTIALS | STUN_AGENT_USAGE_USE_FINGERPRINT
    );

    stun_agent_init(&stunAgent, STUN_ALL_KNOWN_ATTRIBUTES,
                    STUN_COMPATIBILITY_RFC5389, agentUsageFlags);

    // validate request:

    StunDefaultValidaterData validaterData = {
        test::stun_bind_request_ice_ufrag, sizeof(test::stun_bind_request_ice_ufrag) - 1,
        test::stun_bind_request_ice_pwd, sizeof(test::stun_bind_request_ice_pwd) - 1
    };

    StunValidationStatus validationResult = stun_agent_validate(&stunAgent, &stunMsg,
    test::stun_bind_req, test::stun_bind_req_len,
    stun_agent_default_validater, &validaterData);

    CHECK(validationResult == STUN_VALIDATION_SUCCESS);
    CHECK(stun_message_get_class(&stunMsg) == STUN_REQUEST);
    CHECK(stun_message_get_method(&stunMsg) == STUN_BINDING);

    // generate response:

    StunMessage stunResponse;
    std::size_t stunResponseBufLen = 512;
    unsigned char stunResponseBuf[512] = {0};

    struct sockaddr_in from{};
    from.sin_family = AF_INET;
    from.sin_port = htons(3490);
    inet_aton("63.161.169.137", reinterpret_cast<in_addr*>(&from.sin_addr.s_addr));

    bool iceControlling = false;

    auto iceRet = stun_usage_ice_conncheck_create_reply(
        &stunAgent, &stunMsg, &stunResponse, stunResponseBuf,
        &stunResponseBufLen, (const sockaddr_storage*) &from,
        sizeof(from), &iceControlling, 0,
        STUN_USAGE_ICE_COMPATIBILITY_RFC5245);

    CHECK(iceRet == STUN_USAGE_ICE_RETURN_SUCCESS);

    CHECK(stun_message_get_class(&stunResponse) == STUN_RESPONSE);
    CHECK(stun_message_get_method(&stunResponse) == STUN_BINDING);
    CHECK(stun_message_has_attribute(&stunResponse, STUN_ATTRIBUTE_XOR_MAPPED_ADDRESS));
    CHECK(stun_message_has_attribute(&stunResponse, STUN_ATTRIBUTE_MESSAGE_INTEGRITY));
    CHECK(stun_message_has_attribute(&stunResponse, STUN_ATTRIBUTE_FINGERPRINT));

    // access bytes via stunResponse.buffer [0, stun_message_length(&stunResponse) - 1]
}

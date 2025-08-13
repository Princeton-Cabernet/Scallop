
#include "stun_agent.h"

#include <utility>
#include <boost/asio/ip/udp.hpp>

p4sfu::STUNAgent::STUNAgent(Config c)
    : _config{std::move(c)}, _peers{}, _stun{} {

    auto agentUsageFlags = static_cast<StunAgentUsageFlags>(
        STUN_AGENT_USAGE_SHORT_TERM_CREDENTIALS | STUN_AGENT_USAGE_USE_FINGERPRINT
    );

    stun_agent_init(&_stun, STUN_ALL_KNOWN_ATTRIBUTES,
                    STUN_COMPATIBILITY_RFC5389, agentUsageFlags);
}

bool p4sfu::STUNAgent::hasPeer(const net::IPv4Port& ipPort) const {

    return _peers.find(ipPort) != _peers.end();
}

void p4sfu::STUNAgent::addPeer(const net::IPv4Port& ipPort, const stun::Credentials& c) {

    if (!_peers.insert(std::make_pair(ipPort, c)).second)
        throw std::logic_error("STUNAgent: entry already exists");
}

unsigned long p4sfu::STUNAgent::countPeers() const {

    return _peers.size();
}

stun::ValidationResult p4sfu::STUNAgent::validate(const net::IPv4Port& from,
    const unsigned char* reqBuf, std::size_t reqLen) {

    auto peer_it = _peers.find(from);

    if (peer_it == _peers.end()) {
        return stun::ValidationResult{false, false, "UNKNOWN_PEER"};
    }

    StunMessage stunReq;

    std::string username = _config.credentials.uFrag + ":" + peer_it->second.uFrag;
    std::string pwd = _config.credentials.pwd;

    StunDefaultValidaterData validaterData = {
        (unsigned char*) username.c_str(), username.length(),
        (unsigned char*) pwd.c_str(), pwd.length()
    };

    auto valRes = stun_agent_validate(&_stun, &stunReq, reqBuf, reqLen,
                                      stun_agent_default_validater, &validaterData);

    if (valRes == STUN_VALIDATION_SUCCESS) {

        StunMessage stunResp;

        struct sockaddr_in from_sockaddr{};
        from_sockaddr.sin_family      = AF_INET;
        from_sockaddr.sin_port        = htons(from.port());
        from_sockaddr.sin_addr.s_addr = htonl(from.ip().num());

        bool iceControlling = false;

        if (stun_message_has_attribute(&stunReq, STUN_ATTRIBUTE_ICE_CONTROLLED)) {
            iceControlling = true;
        } else if (stun_message_has_attribute(&stunReq, STUN_ATTRIBUTE_ICE_CONTROLLING)) {
            iceControlling = false;
        }

        std::size_t bufLen = MSG_BUF_LEN;

        auto result = stun_usage_ice_conncheck_create_reply(&_stun, &stunReq, &stunResp, _msgBuf,
                                                            &bufLen,
                                                            (const sockaddr_storage*) &from_sockaddr,
                                                            sizeof(from_sockaddr), &iceControlling,
                                                            0, STUN_USAGE_ICE_COMPATIBILITY_RFC5245);

        if (result == STUN_USAGE_ICE_RETURN_SUCCESS) {

            _msgLen = bufLen;

            if (peer_it->second.validated) {
                return stun::ValidationResult{true, false, stun::validationResultString(valRes)};
            } else {
                peer_it->second.validated = true;
                return stun::ValidationResult{true, true, stun::validationResultString(valRes)};
            }
        }
    }

    return stun::ValidationResult{false, false, stun::validationResultString(valRes)};
}

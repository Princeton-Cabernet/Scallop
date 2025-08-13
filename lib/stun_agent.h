
#ifndef P4_SFU_STUN_AGENT_H
#define P4_SFU_STUN_AGENT_H

#include "net/net.h"
#include "proto/stun.h"

namespace p4sfu {

    class STUNAgent {

    public:

        struct Config {
            stun::Credentials credentials;
        };

        explicit STUNAgent(Config c);

        bool hasPeer(const net::IPv4Port& ipPort) const;

        void addPeer(const net::IPv4Port& ipPort, const stun::Credentials& c);

        [[nodiscard]] unsigned long countPeers() const;

        [[nodiscard]] stun::ValidationResult validate(const net::IPv4Port& from,
              const unsigned char* reqBuf, std::size_t reqLen);

        [[nodiscard]] inline const unsigned char* msgBuf() const {
            return _msgBuf;
        }

        [[nodiscard]] inline unsigned msgLen() const {
            return _msgLen;
        }

    private:

        Config _config;
        std::unordered_map<net::IPv4Port, stun::Credentials> _peers;
        StunAgent _stun;

        static const unsigned MSG_BUF_LEN = 512;
        unsigned char _msgBuf[MSG_BUF_LEN] = {};
        unsigned _msgLen = 0;
    };
}

#endif


#ifndef P4_SFU_SFU_CONFIG_H
#define P4_SFU_SFU_CONFIG_H

#include <string>
#include <cstdint>
#include <utility>

#include "net/net.h"
#include "proto/sdp.h"

namespace p4sfu {

    class SFUConfig {

    public:
        SFUConfig() = delete;
        SFUConfig(const SFUConfig&) = default;
        SFUConfig& operator=(const SFUConfig&) = default;

        explicit SFUConfig(const std::string& ip, std::uint16_t port, const std::string& iceUfrag,
                           const std::string& icePwd, net::IPv4Prefix netLimit,
                           unsigned av1ExtensionId = 13)
            : _ip(ip), _port(port), _iceUfrag(iceUfrag), _icePwd(icePwd), _netLimit{netLimit} {

            _sessionTemplate.version               = 0;
            _sessionTemplate.origin.userName       = "-";
            _sessionTemplate.origin.sessionId      = 0;
            _sessionTemplate.origin.sessionVersion = 0;
            _sessionTemplate.origin.netType        = "IN";
            _sessionTemplate.origin.addrType       = "IP4";
            _sessionTemplate.origin.unicastAddr    = ip;
            _iceCandidate.foundation               = "0";
            _iceCandidate.componentId              = 1; // rtp
            _iceCandidate.protocol                 = "udp";
            _iceCandidate.priority                 = 1;
            _iceCandidate.address                  = ip;
            _iceCandidate.port                     = port;
            _iceCandidate.type                     = "host";
            _iceCandidate.remainder                = "generation 0 network-id 1 network-cost 10";

            _videoFbTypes = {"goog-remb", "nack",  "nack pli", "ccm fir"};
            _audioFbTypes = {"goog-remb"};


            _audioRTPExtensions.emplace_back(2,
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time");
            _audioRTPExtensions.emplace_back(9, "urn:ietf:params:rtp-hdrext:sdes:mid");
            _audioRTPExtensions.emplace_back(14, "urn:ietf:params:rtp-hdrext:ssrc-audio-level");

            _videoRTPExtensions.emplace_back(2,
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time");
            _videoRTPExtensions.emplace_back(9, "urn:ietf:params:rtp-hdrext:sdes:mid");
            _videoRTPExtensions.emplace_back(av1ExtensionId,
                "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension");

            _sessionTemplate.otherLines = {
                "a=extmap-allow-mixed",
                "a=msid-semantic: WMS",
            };
        }

        [[nodiscard]] std::string ip() const {
            return _ip;
        }

        [[nodiscard]] std::uint16_t port() const {
            return _port;
        }

        [[nodiscard]] std::string iceUfrag() const {
            return _iceUfrag;
        }

        [[nodiscard]] std::string icePwd() const {
            return _icePwd;
        }

        [[nodiscard]] std::vector<std::string> videoFbTypes() const {
            return _videoFbTypes;
        }

        [[nodiscard]] std::vector<std::string> audioFbTypes() const {
            return _audioFbTypes;
        }

        [[nodiscard]] std::vector<SDP::RTPExtension> audioRTPExtensions() const {
            return _audioRTPExtensions;
        }

        [[nodiscard]] std::vector<SDP::RTPExtension> videoRTPExtensions() const {
            return _videoRTPExtensions;
        }

        [[nodiscard]] const SDP::SessionDescription& sessionTemplate() const {
            return _sessionTemplate;
        }

        [[nodiscard]] const SDP::ICECandidate& iceCandidate() const {
            return _iceCandidate;
        }

        [[nodiscard]] net::IPv4Prefix netLimit() const {
            return _netLimit;
        }

    private:
        std::string _ip;
        std::uint16_t _port;
        std::string _iceUfrag;
        std::string _icePwd;
        SDP::SessionDescription _sessionTemplate;
        SDP::ICECandidate _iceCandidate;
        std::vector<std::string> _videoFbTypes;
        std::vector<std::string> _audioFbTypes;
        std::vector<SDP::RTPExtension> _audioRTPExtensions;
        std::vector<SDP::RTPExtension> _videoRTPExtensions;
        net::IPv4Prefix _netLimit;
    };
}

#endif

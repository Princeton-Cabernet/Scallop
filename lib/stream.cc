
#include <cassert>
#include "stream.h"
#include "util.h"

p4sfu::Stream::Stream(const SDP::MediaDescription& media, const p4sfu::SFUConfig& sfuConfig,
                      const SDP::SSRCDescription& ssrcDescription)
    : _description{media} {

    if (!_description.direction)
        throw std::logic_error("Stream: Stream: direction not set in description");

    if (*(_description.direction) == SDP::Direction::sendonly) {

        if (!ssrcDescription.ssrcValues().empty())
            throw std::invalid_argument("Stream: Stream(): SSRCs can only be provided in "
                + std::string("recvonly streams"));

        if (!_description.ssrcDescription)
            throw std::logic_error("Stream: Stream: ssrc description not set in sendonly stream");

        auto filtered =
                SDP::filterCandidatesBySubnet(_description.iceCandidates, sfuConfig.netLimit());

        if (filtered.empty())
            throw std::invalid_argument("Stream: Stream(): no ICE candidates matching subnet filter");

        auto highest = SDP::highestPriorityICECandidate(filtered);

        SendStreamConfig ss;
        ss.addr = net::IPv4Port{net::IPv4{highest.address}, highest.port};
        ss.iceUfrag = *(_description.iceUfrag);
        ss.icePwd = *(_description.icePwd);

        ss.mediaType = _description.type == SDP::MediaDescription::Type::audio ?
                       MediaType::audio : MediaType::video;

        ss.mainSSRC = _description.ssrcDescription->ssrcs[0].ssrc;

        if (_description.ssrcDescription->ssrcs.size() > 1) {
            ss.rtxSSRC = _description.ssrcDescription->ssrcs[1].ssrc;
        } else {
            ss.rtxSSRC = 0;
        }

        _dataPlaneConfig = ss;

    } else if (*(_description.direction) == SDP::Direction::recvonly) {

        if (ssrcDescription.ssrcValues().empty())
            throw std::invalid_argument("Stream: Stream(): SSRCs must be provided in "
                + std::string("recvonly streams"));

        auto filtered =
                SDP::filterCandidatesBySubnet(_description.iceCandidates, sfuConfig.netLimit());

        if (filtered.empty())
            throw std::invalid_argument("Stream: Stream(): no ICE candidates matching subnet filter");

        auto highest = SDP::highestPriorityICECandidate(filtered);

        ReceiveStreamConfig rs;
        rs.addr = net::IPv4Port{net::IPv4{highest.address}, highest.port};
        rs.iceUfrag = *(_description.iceUfrag);
        rs.icePwd = *(_description.icePwd);

        rs.mediaType = _description.type == SDP::MediaDescription::Type::audio ?
                       MediaType::audio : MediaType::video;

        // use SSRCs of corresponding send-stream (supplied through ssrcDescription)
        rs.mainSSRC = ssrcDescription.ssrcs[0].ssrc;

        if (ssrcDescription.ssrcs.size() > 1) {
            rs.rtxSSRC = ssrcDescription.ssrcs[1].ssrc;
        } else {
            rs.rtxSSRC = 0;
        }

        if (_description.ssrcDescription.has_value()) {
            // take SSRC from this description as the SSRC used for RTCP
            rs.rtcpSSRC = _description.ssrcDescription->ssrcs[0].ssrc;

            if (_description.ssrcDescription->ssrcs.size() > 1) {
                rs.rtcpRtxSSRC = _description.ssrcDescription->ssrcs[1].ssrc;
            }
        } else {

            // set rtcp ssrcs to one if not provided (recvonly stream w/o ssrc desc.):
            //  - this is the case when using new transceivers instead of adding streams directly
            //    to the peer connection
            //  - RTCP feedback from transceivers that do not send media uses sender SSRC = 1
            rs.rtcpSSRC = 1;
            rs.rtcpRtxSSRC = 1;
        }

        _dataPlaneConfig = rs;

    } else {
        throw std::invalid_argument("Stream: Stream: unsupported direction");
    }

    /* use this to remove fmtp lines from SDP:
    for (auto& fmt: _description.mediaFormats) {
        if (fmt.codec == "AV1") {
            fmt.fmtParams.clear();
            std::cout << ": " << std::endl;
            for (auto& fmtParam: fmt.fmtParams) {
                std::cout << "FORMAT PARAM: " << fmtParam << std::endl;
            }
        }
    }
    */

}

const SDP::MediaDescription& p4sfu::Stream::description() const {

    return _description;
}

SDP::MediaDescription p4sfu::Stream::offer(const p4sfu::SFUConfig& sfuConfig) const {

    // modify offer such that SFU is the originating peer:
    auto offer = _description;

    offer.port = sfuConfig.port();
    offer.connectionData->connectionAddress = sfuConfig.ip();
    offer.iceCandidates.clear();
    offer.iceCandidates.push_back(sfuConfig.iceCandidate());
    offer.iceUfrag = sfuConfig.iceUfrag();
    offer.icePwd = sfuConfig.icePwd();

    return offer;
}

const std::variant<p4sfu::Stream::SendStreamConfig, p4sfu::Stream::ReceiveStreamConfig>&
        p4sfu::Stream::dataPlaneConfig() const {

    return _dataPlaneConfig;
}

std::string p4sfu::Stream::formattedSummary() const {

    std::stringstream ss;
    ss << "type=" << SDP::typeString(_description.type) << ", ice-addr="
       << _description.highestPriorityICECandidate().address << ":"
       << _description.highestPriorityICECandidate().port << ", ssrc=["
       << util::joinStrings(_description.ssrcDescription->ssrcValues(), ",") << "]";

    return ss.str();
}

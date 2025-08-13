
#ifndef P4_SFU_SDP_H
#define P4_SFU_SDP_H

#include <cstdint>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include "../net/net.h"

namespace SDP {

    enum class Direction {
        recvonly = 0,
        sendrecv = 1,
        sendonly = 2,
        inactive = 3
    };

    class Origin { // https://www.rfc-editor.org/rfc/rfc4566#section-5.2
    public:

        inline static std::string O_LINE_REGEX = R"(o=([\w\d-]+) ([\d]+) ([\d]+) ([\w\d]+) ([\w\d]+) ([\d:\.]+))";

        std::string userName = "-";
        unsigned long sessionId = 0;
        unsigned long sessionVersion = 0;
        std::string netType = "IN";
        std::string addrType = "IP4";
        std::string unicastAddr = "0.0.0.0";

        Origin() = default;
        explicit Origin(const std::string& oLine);
        [[nodiscard]] std::string line() const;
    };

    class Timing { // https://www.rfc-editor.org/rfc/rfc4566#section-5.9
    public:
        inline static std::string T_LINE_REGEX = R"(t=(\d+) (\d+))";

        unsigned long start = 0;
        unsigned long stop  = 0;

        Timing() = default;
        explicit Timing(const std::string& tLine);
        [[nodiscard]] std::string line() const;
    };

    class ConnectionData { // https://www.rfc-editor.org/rfc/rfc4566#section-5.7

    public:
        inline static std::string C_LINE_REGEX = R"(c=(IN) (IP4|IP6) (.+))";

        std::string netType;
        std::string addrType;
        std::string connectionAddress;

        ConnectionData() = default;

        ConnectionData(const std::string& netType, const std::string& addrType,
                       const std::string& connectionAddress)
            : netType(netType), addrType(addrType), connectionAddress(connectionAddress) { }

        explicit ConnectionData(const std::string& cLine);
        [[nodiscard]] std::string line() const;
    };

    class SSRCDescription {

    public:
        inline static std::string SSRC_REGEX = R"(a=ssrc.+)";
        inline static std::string SSRC_GROUP_REGEX = R"(a=ssrc-group:(FID|FEC) ((?:[0-9]+)(?:\s[0-9]+)+))";
        inline static std::string SSRC_ATTR_REGEX = R"(a=ssrc:([0-9]+) (msid|cname):([\w\s\-+]+))";

        struct SSRC {
            std::uint32_t ssrc = 0;
            std::string   cname;
            std::string   msid;
        };

        SSRCDescription() = default;

        explicit SSRCDescription(std::vector<SSRC> ssrcs)
            : ssrcs(std::move(ssrcs)) { }
        explicit SSRCDescription(const std::vector<std::string>& ssrcLines);
        std::vector<SSRC> ssrcs = {};
        [[nodiscard]] std::vector<std::string> lines() const;
        [[nodiscard]] std::vector<std::uint32_t> ssrcValues() const;
    };

    class ICECandidate {

    public:

        inline static std::string ICE_CANDIDATE_REGEX = R"(a=candidate:([0-9a-z\/\+]+) (\d{1,3}) (tcp|udp) (\d+) (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}) (\d{1,5}) typ (host|srflx|prflx|relay)(?:\s?(.*)))";

        ICECandidate() = default;
        explicit ICECandidate(const std::string& line);

        [[nodiscard]] std::string line() const;

        std::string foundation;
        unsigned componentId = 0;
        std::string protocol;
        unsigned long priority = 0;
        std::string address;
        unsigned short port = 0;
        std::string type;
        std::string remainder;

        bool operator==(const ICECandidate& other) const;
    };

    class MediaFormat {

    public:
        inline static std::string MEDIA_FORMAT_REGEX = R"(a=(?:rtpmap|rtcp-fb|fmtp).+)";
        inline static std::string RTP_MAP_REGEX = R"(a=rtpmap:([\d]+) ([\w\d]+)\/([\d]+)(?:\/([\d]+)?)?)";
        inline static std::string RTCP_FB_REGEX = R"(a=rtcp-fb:([\d]+) ([\w\s-]+))";
        inline static std::string RTP_FMTP_REGEX = R"(a=fmtp:([\d]+) (.+))";

        explicit MediaFormat(const std::vector<std::string>& lines);

        explicit MediaFormat(unsigned pt, const std::string& codec, unsigned clockRate)
            : pt(pt), codec(codec), clockRate(clockRate) { }

        [[nodiscard]] std::vector<std::string> lines() const;

        unsigned pt = 0;
        std::string codec;
        unsigned clockRate = 0;
        std::optional<unsigned> channels = std::nullopt;

        std::vector<std::string> fbTypes;
        std::vector<std::string> fmtParams;

        static std::string payloadTypesString(const std::vector<MediaFormat>& formats);
    };

    class RTPExtension {
    public:
        inline static std::string EXTMAP_REGEX = R"(a=extmap:(\d+) (.+))";

        explicit RTPExtension(const std::string& line);
        RTPExtension(unsigned identifier, const std::string& uri)
            : identifier(identifier), uri(uri) { }

        [[nodiscard]] std::string line() const;

        unsigned identifier;
        std::string uri;
    };

    class MediaStreamIdentifier {
    public:
        inline static std::string MSID_REGEX = R"(a=msid:(\S+)(?:\s(\S+))?)";

        MediaStreamIdentifier(std::string identifier, std::string appData)
            : identifier(std::move(identifier)), appData(std::move(appData)) { }

        explicit MediaStreamIdentifier(const std::string& line);
        [[nodiscard]] std::string line() const;

        std::string identifier;
        std::string appData;
    };

    class RTCPConfiguration { // https://www.rfc-editor.org/rfc/rfc3605#section-2.1
    public:

        inline static std::string RTCP_REGEX = R"(a=rtcp:(\d+)(?: (IN) (IP4) (.+))?)";

        explicit RTCPConfiguration(const std::string& line);

        RTCPConfiguration(unsigned short port, std::string  netType,
                          std::string  addrType, std::string  connectionAddress)
            : port(port), netType(std::move(netType)), addrType(std::move(addrType)),
              connectionAddress(std::move(connectionAddress)) { }

        [[nodiscard]] std::string line() const;

        unsigned short port = 9;
        std::string netType;
        std::string addrType;
        std::string connectionAddress;
    };

    class MediaDescription {
    public:

        enum class Type {
            audio = 0,
            video = 1
        };

        inline static std::string M_LINE_REGEX = R"(m=(audio|video)\s(\d{1,5})\s([\/\w]+)\s([\d{1,3}\s?]+))";
        inline static std::string ICE_UFRAG_REGEX = R"(a=ice-ufrag:([\w\+\/:]+))";
        inline static std::string ICE_PWD_REGEX = R"(a=ice-pwd:([\w\+\/:]+))";
        inline static std::string MID_REGEX = R"(a=mid:([\w]+))";
        inline static std::string DIRECTION_REGEX = R"(a=(sendrecv|sendonly|recvonly|inactive))";

        inline static std::string ICE_OTHER_REGEX = R"(a=ice-.+)";
        inline static std::string RTCP_OTHER_REGEX = R"(a=rtcp-(mux|rsize))";

        MediaDescription() = default;

        explicit MediaDescription(const std::vector<std::string>& lines);

        [[nodiscard]] ICECandidate highestPriorityICECandidate() const;

        [[nodiscard]] std::vector<std::string> lines() const;

        Type type;
        unsigned short port = 0;
        std::string protocols;

        std::optional<ConnectionData> connectionData;
        std::optional<RTCPConfiguration> rtcpConfiguration;
        std::vector<ICECandidate> iceCandidates;
        std::vector<MediaFormat> mediaFormats;
        std::optional<std::string> iceUfrag = std::nullopt;
        std::optional<std::string> icePwd = std::nullopt;

        std::vector<std::string> iceOtherLines = {};

        std::optional<std::string> mid = std::nullopt;
        std::optional<Direction> direction = std::nullopt;

        std::vector<RTPExtension> extensions;
        std::optional<MediaStreamIdentifier> msid = std::nullopt;

        std::vector<std::string> rtcpOtherLines = {};

        std::optional<SSRCDescription> ssrcDescription = std::nullopt;

        static MediaDescription::Type typeFromString(const std::string& s);
        static std::string stringFromType(const MediaDescription::Type& t);

    private:
        std::vector<std::string> _otherLines = {};
    };

    class SessionDescription { // https://www.rfc-editor.org/rfc/rfc4566#section-5

    public:

        inline static std::string V_LINE_REGEX = R"(v=(\d+))";
        inline static std::string S_LINE_REGEX = R"(s=(.+))";
        inline static std::string BUNDLE_REGEX = R"(a=group:BUNDLE.+)";

        SessionDescription();
        explicit SessionDescription(const std::string& sdpString);
        explicit SessionDescription(const std::vector<std::string>& lines);

        [[nodiscard]] std::vector<std::string> lines() const;
        [[nodiscard]] std::string formattedSummary() const;
        [[nodiscard]] std::string escapedString() const;

        unsigned version = 0;
        Origin origin;
        std::string sessionName = "-";
        std::vector<Timing> timing;
        std::vector<MediaDescription> mediaDescriptions;
        std::vector<std::string> otherLines = {};
    };

    std::string bundleLine(unsigned count);
    Direction directionFromString(const std::string& s);
    std::string directionString(const Direction& d);
    std::string typeString(const MediaDescription::Type& t);

    std::vector<ICECandidate> filterCandidatesBySubnet(const std::vector<ICECandidate>& candidates,
                                                       const net::IPv4Prefix& net);

    ICECandidate highestPriorityICECandidate(const std::vector<ICECandidate>& candidates);
}

#endif

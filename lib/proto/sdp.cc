
#include <algorithm>
#include "sdp.h"
#include "../util.h"

//const std::regex SDP::Origin::O_LINE_REGEX
//    = std::regex(R"(o=([\w\d-]+) ([\d]+) ([\d]+) ([\w\d]+) ([\w\d]+) ([\d:\.]+))");

SDP::Origin::Origin(const std::string& oLine) {

    std::smatch match;

    if (std::regex_match(oLine, match, std::regex{O_LINE_REGEX}) && match.size() == 7) {
        userName = match[1];
        sessionId = std::stoul(match[2]);
        sessionVersion = std::stoul(match[3]);
        netType = match[4];
        addrType = match[5];
        unicastAddr = match[6];
    }
}

[[nodiscard]] std::string SDP::Origin::line() const {

    return util::formatString("o=%s %lu %lu %s %s %s", userName.c_str(), sessionId,
                              sessionVersion, netType.c_str(), addrType.c_str(),
                              unicastAddr.c_str());
}

//const std::regex SDP::Timing::T_LINE_REGEX
//    = std::regex(R"(t=(\d+) (\d+))");


SDP::Timing::Timing(const std::string& tLine) {

    std::smatch match;

    if (std::regex_match(tLine, match, std::regex{T_LINE_REGEX}) && match.size() == 3) {
        start = std::stoul(match[1]);
        stop  = std::stoul(match[2]);
    }
}

std::string SDP::Timing::line() const {

    std::stringstream tLine;
    tLine << "t=" << start << " " << stop;
    return tLine.str();
}


//const std::regex SDP::ConnectionData::C_LINE_REGEX = std::regex(R"(c=(IN) (IP4|IP6) (.+))");

SDP::ConnectionData::ConnectionData(const std::string& cLine) {

    std::smatch match;

    if (std::regex_match(cLine, match, std::regex{C_LINE_REGEX}) && match.size() == 4) {
        netType = match[1];
        addrType = match[2];
        connectionAddress = match[3];
    }
}

std::string SDP::ConnectionData::line() const {

    return util::formatString("c=%s %s %s", netType.c_str(), addrType.c_str(),
                              connectionAddress.c_str());
}

//
//const std::regex SDP::SSRCDescription::SSRC_REGEX = std::regex(R"(a=ssrc.+)");
//
//const std::regex SDP::SSRCDescription::SSRC_GROUP_REGEX
//    = std::regex(R"(a=ssrc-group:(FID|FEC) ((?:[0-9]+)(?:\s[0-9]+)+))");
//
//const std::regex SDP::SSRCDescription::SSRC_ATTR_REGEX
//    = std::regex(R"(a=ssrc:([0-9]+) (msid|cname):([\w\s\-+]+))");

SDP::SSRCDescription::SSRCDescription(const std::vector<std::string> &ssrcLines) {

    std::smatch match;

    for (const std::string& line : ssrcLines) {

        if (std::regex_match(line, match, std::regex{SSRC_GROUP_REGEX}) && match.size() == 3) {

            for (const std::string& part: util::splitString(match[2], " ")) {
                ssrcs.push_back({ .ssrc = (std::uint32_t) std::stoul(part) });
            }

        } else if (std::regex_match(line, match, std::regex{SSRC_ATTR_REGEX}) && match.size() == 4) {

            std::uint32_t ssrc =  std::stoul(match[1]);

            auto it = std::find_if(ssrcs.begin(), ssrcs.end(),[ssrc](const SSRC& s) {
                return s.ssrc == ssrc;
            });

            if (it == ssrcs.end()) {
                it = ssrcs.insert(ssrcs.end(), { .ssrc = ssrc });
            }

            if (match[2] == "cname") {
                it->cname = match[3];
            } else if (match[2] == "msid") {
                it->msid = match[3];
            }
        }
    }
}

std::vector<std::string> SDP::SSRCDescription::lines() const {

    std::vector<std::string> lines;

    if (ssrcs.size() > 1) { // ssrc group line

        std::vector<std::string> ssrcValues(ssrcs.size());

        std::transform(ssrcs.begin(), ssrcs.end(), ssrcValues.begin(),
            [](const SSRC& s) -> std::string { return std::to_string(s.ssrc); });

        lines.push_back(util::formatString("a=ssrc-group:FID %s",
                                           util::joinStrings(ssrcValues).c_str()));
    }

    for (const auto& ssrc: ssrcs) { // ssrc attribute lines

        if (!ssrc.cname.empty())
            lines.push_back(util::formatString("a=ssrc:%u cname:%s", ssrc.ssrc, ssrc.cname.c_str()));

        if (!ssrc.msid.empty())
            lines.push_back((util::formatString("a=ssrc:%u msid:%s", ssrc.ssrc, ssrc.msid.c_str())));
    }

    return lines;
}

[[nodiscard]] std::vector<std::uint32_t> SDP::SSRCDescription::ssrcValues() const {

    std::vector<uint32_t> values(ssrcs.size());

    std::transform(ssrcs.begin(), ssrcs.end(), values.begin(), [](const auto& ssrc) {
        return ssrc.ssrc;
    });

    return values;
}

//const std::regex SDP::ICECandidate::ICE_CANDIDATE_REGEX
//    = std::regex(R"(a=candidate:([0-9a-z\/\+]+) (\d{1,3}) (tcp|udp) (\d+) (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}) (\d{1,5}) typ (host|srflx|prflx|relay)(?:\s?(.*)))");

SDP::ICECandidate::ICECandidate(const std::string& line) {

    std::smatch match;

    if (std::regex_match(line, match, std::regex{ICE_CANDIDATE_REGEX}) && match.size() >= 8) {

        foundation = match[1];
        componentId = std::stoul(match[2]);
        protocol = match[3];
        priority = std::stoul(match[4]);
        address = match[5];
        port = std::stoul(match[6]);
        type = match[7];

        if (match.size() == 9) {
            remainder = match[8];
        }
    }
}

std::string SDP::ICECandidate::line() const {

    auto base = util::formatString("a=candidate:%s %u %s %u %s %u typ %s",
        foundation.c_str(), componentId, protocol.c_str(), priority,
        address.c_str(), port, type.c_str());

    if (!remainder.empty())
        return base + " " + remainder;

    return base;
}

bool SDP::ICECandidate::operator==(const SDP::ICECandidate& other) const {

    return foundation  == other.foundation
        && componentId == other.componentId
        && protocol    == other.protocol
        && priority    == other.priority
        && address     == other.address
        && port        == other.port
        && type        == other.type
        && remainder   == other.remainder;
}

//
//const std::regex SDP::MediaFormat::MEDIA_FORMAT_REGEX
//    = std::regex(R"(a=(?:rtpmap|rtcp-fb|fmtp).+)");
//
//const std::regex SDP::MediaFormat::RTP_MAP_REGEX
//    = std::regex(R"(a=rtpmap:([\d]+) ([\w\d]+)\/([\d]+))");
//
//const std::regex SDP::MediaFormat::RTCP_FB_REGEX
//    = std::regex(R"(a=rtcp-fb:([\d]+) ([\w\s-]+))");
//
//const std::regex SDP::MediaFormat::RTP_FMTP_REGEX
//    = std::regex(R"(a=fmtp:([\d]+) (.+))");

SDP::MediaFormat::MediaFormat(const std::vector<std::string>& lines) {

    std::smatch match;

    for (const auto& line: lines) {

        if (std::regex_match(line, match, std::regex{RTP_MAP_REGEX}) && match.size() == 5) {

            pt = std::stoul(match[1]);
            codec = match[2];
            clockRate = std::stoul(match[3]);

            if (match[4].matched) {
                channels = std::stoul(match[4]);
            }

        } else if (std::regex_match(line, match, std::regex{RTCP_FB_REGEX}) && match.size() == 3) {

            if (std::stoul(match[1]) == pt) {
                fbTypes.push_back(match[2]);
            }

        } else if (std::regex_match(line, match, std::regex{RTP_FMTP_REGEX}) && match.size() == 3) {

            if (std::stoul(match[1]) == pt) {
                fmtParams.push_back(match[2]);
            }
        }
    }
}

std::vector<std::string> SDP::MediaFormat::lines() const {

    std::vector<std::string> lines;

    if (channels.has_value()) {
        lines.push_back(util::formatString("a=rtpmap:%u %s/%u/%u", pt, codec.c_str(), clockRate,
                                           *channels));
    } else {
        lines.push_back(util::formatString("a=rtpmap:%u %s/%u", pt, codec.c_str(), clockRate));
    }

    for (const auto& fbType: fbTypes) {
        lines.push_back(util::formatString("a=rtcp-fb:%u %s", pt, fbType.c_str()));
    }

    for (const auto& fmtParam: fmtParams) {
        lines.push_back(util::formatString("a=fmtp:%u %s", pt, fmtParam.c_str()));
    }

    return lines;
}

std::string SDP::MediaFormat::payloadTypesString(const std::vector<MediaFormat>& formats) {

    std::stringstream ss;

    for (auto i = 0; i < formats.size(); i++)
        ss << formats[i].pt << (i != formats.size()-1 ? " " : "");

    return ss.str();
}

//const std::regex SDP::RTPExtension::EXTMAP_REGEX
//        = std::regex(R"(a=extmap:(\d+) (.+))");


SDP::RTPExtension::RTPExtension(const std::string& line) {

    std::smatch matches;

    if (std::regex_match(line, matches, std::regex{EXTMAP_REGEX}) && matches.size() == 3) {
        identifier = std::stoul(matches[1]);
        uri = matches[2];
    }
}

std::string SDP::RTPExtension::line() const {

    return util::formatString("a=extmap:%u %s", identifier, uri.c_str());
}


//const std::regex SDP::MediaStreamIdentifier::MSID_REGEX
//    = std::regex(R"(a=msid:(\S+)(?:\s(\S+))?)");

SDP::MediaStreamIdentifier::MediaStreamIdentifier(const std::string &line) {

    std::smatch matches;

    if (std::regex_match(line, matches, std::regex{MSID_REGEX}) && matches.size() >= 2) {

        identifier = matches[1];

        if (matches.size() == 3) {
            appData = matches[2];
        }
    }
}

std::string SDP::MediaStreamIdentifier::line() const {

    std::stringstream ss;
    ss << "a=msid:" << identifier;

    if (!appData.empty())
        ss << " " << appData;

    return ss.str();
}


//const std::regex SDP::RTCPConfiguration::RTCP_REGEX
//    = std::regex(R"(a=rtcp:(\d+)(?: (IN) (IP4) (.+))?)");

SDP::RTCPConfiguration::RTCPConfiguration(const std::string& line) {

    std::smatch matches;

    if (std::regex_match(line, matches, std::regex{RTCP_REGEX}) && matches.size() >= 2) {

        port = (unsigned short) std::stoul(matches[1]);

        if (matches.size() == 5) {
            netType = matches[2];
            addrType = matches[3];
            connectionAddress = matches[4];
        }
    }
}

std::string SDP::RTCPConfiguration::line() const {

    std::stringstream ss;

    ss << "a=rtcp:" << port;

    if (!netType.empty() && !addrType.empty() && !connectionAddress.empty()) {
        ss << " " << netType << " " << addrType << " " << connectionAddress;
    }

    return ss.str();
}

/*
const std::regex SDP::MediaDescription::M_LINE_REGEX
    = std::regex(R"(m=(audio|video)\s(\d{1,5})\s([\/\w]+)\s([\d{1,3}\s?]+))");

const std::regex SDP::MediaDescription::ICE_UFRAG_REGEX
    = std::regex(R"(a=ice-ufrag:([\w\+\/:]+))");

const std::regex SDP::MediaDescription::ICE_PWD_REGEX
    = std::regex(R"(a=ice-pwd:([\w\+\/:]+))");

const std::regex SDP::MediaDescription::MID_REGEX
    = std::regex(R"(a=mid:([\w]+))");

const std::regex SDP::MediaDescription::DIRECTION_REGEX
    = std::regex(R"(a=(sendrecv|sendonly|recvonly|inactive))");


const std::regex SDP::MediaDescription::ICE_OTHER_REGEX
     = std::regex(R"(a=ice-.+)");

const std::regex SDP::MediaDescription::RTCP_OTHER_REGEX
     = std::regex(R"(a=rtcp-(mux|rsize))");
*/

SDP::MediaDescription::MediaDescription(const std::vector<std::string>& lines) {

    std::vector<std::string> ssrcLines;
    std::vector<std::string> iceCandidateLines;
    std::vector<std::string> currentFormatLines = {};

    std::smatch matches;

    for (const auto& line: lines) {

        if (std::regex_match(line, matches, std::regex{M_LINE_REGEX}) && matches.size() == 5) {
            type = typeFromString(matches[1]);
            port = std::stoul(matches[2]);
            protocols = matches[3];

        } else if (std::regex_match(line, matches, std::regex{ConnectionData::C_LINE_REGEX})
            && !matches.empty()) {

            connectionData = ConnectionData(line);

        } else if (std::regex_match(line, matches, std::regex{RTCPConfiguration::RTCP_REGEX})
            && !matches.empty()) {

            rtcpConfiguration = RTCPConfiguration(line);

        } else if (std::regex_match(line, matches, std::regex{ICE_UFRAG_REGEX}) && matches.size() == 2) {

            iceUfrag = matches[1];

        } else if (std::regex_match(line, matches, std::regex{ICE_PWD_REGEX}) && matches.size() == 2) {

            icePwd = matches[1];

        } else if (std::regex_match(line, matches, std::regex{MID_REGEX}) && matches.size() == 2) {

            mid = matches[1];

        } else if (std::regex_match(line, matches, std::regex{RTPExtension::EXTMAP_REGEX})
            && matches.size() == 3) {

            extensions.emplace_back(line);

        } else if (std::regex_match(line, matches, std::regex{MediaStreamIdentifier::MSID_REGEX})
            && !matches.empty()) {

            msid = MediaStreamIdentifier(line);
        } else if (std::regex_match(line, matches, std::regex{DIRECTION_REGEX})
            && matches.size() == 2) {

            direction = SDP::directionFromString(matches[1]);

        } else if (std::regex_match(line, matches, std::regex{ICECandidate::ICE_CANDIDATE_REGEX})) {

            iceCandidateLines.push_back(line);

        } else if (std::regex_match(line, matches, std::regex{SSRCDescription::SSRC_REGEX})) {

            ssrcLines.push_back(line);

        } else if (std::regex_match(line, matches, std::regex{MediaFormat::MEDIA_FORMAT_REGEX})
            && !matches.empty()) {

            if (std::regex_match(line, matches, std::regex{MediaFormat::RTP_MAP_REGEX})
                && matches.size() == 5) {

                if (!currentFormatLines.empty()) {
                    mediaFormats.emplace_back(currentFormatLines);
                    currentFormatLines = {};
                }
            }

            currentFormatLines.push_back(line);

        } else if (std::regex_match(line, matches, std::regex{ICE_OTHER_REGEX}) && !matches.empty()) {

            iceOtherLines.push_back(line); // must be at end

        } else if (std::regex_match(line, matches, std::regex{RTCP_OTHER_REGEX}) && !matches.empty()) {

            rtcpOtherLines.push_back(line);

        } else {

            _otherLines.push_back(line);
        }
    }

    // add format left in currentFormatLines
    if (!currentFormatLines.empty()) {
        mediaFormats.emplace_back(currentFormatLines);
    }

    for (const auto& iceCandidateLine: iceCandidateLines)
        iceCandidates.emplace_back(iceCandidateLine);

    if (!ssrcLines.empty())
        ssrcDescription = SSRCDescription(ssrcLines);
}

SDP::ICECandidate SDP::MediaDescription::highestPriorityICECandidate() const {

    if (!iceCandidates.empty()) {

        auto all = iceCandidates;

        std::sort(all.begin(), all.end(), [](const auto& a, const auto& b) {
           return a.priority < b.priority;
        });

        return all[all.size()-1];

    } else {
        throw std::logic_error("MediaDescription::highestPriorityICECandidate: no ice candidates");
    }
}

std::vector<std::string> SDP::MediaDescription::lines() const {

    std::vector<std::string> lines = {};

    lines.push_back(util::formatString("m=%s %u %s %s",
        stringFromType(type).c_str(), port, protocols.c_str(),
        MediaFormat::payloadTypesString(mediaFormats).c_str()));

    if (connectionData)
        lines.push_back(connectionData->line());

    if (rtcpConfiguration)
        lines.push_back(rtcpConfiguration->line());

    for (const auto& c: iceCandidates)
        lines.push_back(c.line());

    if (iceUfrag)
        lines.push_back(util::formatString("a=ice-ufrag:%s", iceUfrag->c_str()));

    if (icePwd)
        lines.push_back(util::formatString("a=ice-pwd:%s", icePwd->c_str()));

    for (const auto& line: iceOtherLines)
        lines.push_back(line);

    if (mid)
        lines.push_back(util::formatString("a=mid:%s", mid->c_str()));

    for (const auto& ext: extensions)
        lines.push_back(ext.line());

    if (direction)
        lines.push_back(util::formatString("a=%s", SDP::directionString(*direction).c_str()));

    if (msid)
        lines.push_back(msid->line());

    for (const auto& line: rtcpOtherLines)
        lines.push_back(line);

    for (const auto& format: mediaFormats) {
        for (const auto& line: format.lines()) {
            lines.push_back(line);
        }
    }

    if (ssrcDescription) {
        for (const auto& line: ssrcDescription->lines())
            lines.push_back(line);
    }

    return lines;
}

SDP::MediaDescription::Type SDP::MediaDescription::typeFromString(const std::string& s) {

    if (s == "audio") {
        return Type::audio;
    } else if (s == "video") {
        return Type::video;
    } else {
        throw std::invalid_argument(
            "SDP::MediaDescription::typeFromString: could not parse media type.");
    }
}

std::string SDP::MediaDescription::stringFromType(const MediaDescription::Type& t) {

    switch(t) {
        case Type::audio: return "audio";
        case Type::video: return "video";
    }

    return "";
}

//const std::regex SDP::SessionDescription::V_LINE_REGEX = std::regex(R"(v=(\d+))");

//const std::regex SDP::SessionDescription::S_LINE_REGEX = std::regex(R"(s=(.+))");

//const std::regex SDP::SessionDescription::BUNDLE_REGEX = std::regex(R"(a=group:BUNDLE.+)");

SDP::SessionDescription::SessionDescription() {

    Timing t;
    t.start = 0, t.stop = 0;
    timing.push_back(t);
}

SDP::SessionDescription::SessionDescription(const std::string& sdpString)
    : SessionDescription(util::splitString(sdpString, "\r\n")) { }

SDP::SessionDescription::SessionDescription(const std::vector<std::string>& lines) {

    auto currentMediaSection = -1, mediaSectionCount = 0;
    std::vector<std::string> mediaSectionLines = {};
    std::smatch matches;

    struct {
        bool version = false;
        bool origin  = false;
        bool name    = false;
    } parsed;

    for (auto i = 0; i < lines.size(); i++) {

        if (std::regex_match(lines[i], matches, std::regex{MediaDescription::M_LINE_REGEX})) {

            currentMediaSection = mediaSectionCount;
        }

        if (currentMediaSection >= 0) {

            mediaSectionLines.push_back(lines[i]);

            if ((i == lines.size()-1) || (i < lines.size()-1
                && std::regex_match(lines[i+1], matches, std::regex{MediaDescription::M_LINE_REGEX}))) {

                mediaDescriptions.emplace_back(mediaSectionLines);
                currentMediaSection = -1;
                mediaSectionCount++;
                mediaSectionLines.clear();
            }

        } else {

            if (std::regex_match(lines[i], matches, std::regex{V_LINE_REGEX}) && matches.size() == 2) {
                version = std::stoul(matches[1]);
                parsed.version = true;
            } else if (std::regex_match(lines[i], matches, std::regex{Origin::O_LINE_REGEX})) {
                origin = SDP::Origin(lines[i]);
                parsed.origin = true;
            } else if (std::regex_match(lines[i], matches, std::regex{S_LINE_REGEX})) {
                sessionName = matches[1];
            } else if (std::regex_match(lines[i], matches, std::regex{Timing::T_LINE_REGEX})) {
                timing.emplace_back(lines[i]);
            } else if (std::regex_match(lines[i], matches, std::regex{BUNDLE_REGEX})) {
                // do not include bundle lines as it is reconstructed based on media descriptions
            } else {
                otherLines.push_back(lines[i]);
            }
        }
    }

    if (!parsed.version || version != 0) {
        throw std::invalid_argument("SessionDescription: invalid sdp: no v-line or version != 0");
    }

    if (!parsed.origin) {
        throw std::invalid_argument("SessionDescription: invalid sdp: no o-line");
    }

    if (timing.empty()) {
        throw std::invalid_argument("SessionDescription: invalid sdp: no t-line");
    }
}

std::vector<std::string> SDP::SessionDescription::lines() const {

    std::vector<std::string> lines = {};

    lines.push_back("v=" + std::to_string(version));
    lines.push_back(origin.line());
    lines.push_back("s=" + sessionName);

    for (const auto& tLine: timing) {
        lines.push_back(tLine.line());
    }

    // bundle:
    if (!mediaDescriptions.empty()) {

        std::stringstream bundleLine;
        bundleLine << "a=group:BUNDLE";

        for (const auto& media: mediaDescriptions) {
            if (!media.mid) {
                throw std::logic_error("SessionDescription: lines() description has media w/o mid");
            } else {
                bundleLine << " " << *media.mid;
            }
        }

        lines.push_back(bundleLine.str());
    }

    for (const auto& line: otherLines)
        lines.push_back(line);

    for (const auto& media: mediaDescriptions)
        for (const auto& line: media.lines())
            lines.push_back(line);

    return lines;
}

std::string SDP::SessionDescription::escapedString() const {

    std::stringstream ss;

    for (const auto& line: lines())
        ss << line << "\r\n";

    return ss.str();
}

std::string SDP::SessionDescription::formattedSummary() const {

    std::stringstream ss;

    ss << " - session: id=" << origin.sessionId << ", version="
              << origin.sessionVersion << std::endl;

    for (auto mediaIndex = 0; mediaIndex < mediaDescriptions.size(); mediaIndex++) {

        const auto& media = mediaDescriptions[mediaIndex];

        ss << " - media: type=" << MediaDescription::stringFromType(media.type) << ", port="
            << media.port;

        if (media.direction)
            ss << ", dir=" << SDP::directionString(*media.direction);

        if (media.mid)
            ss << ", mid=" << *media.mid;

        if (media.msid && !media.msid->appData.empty())
            ss << ", trackId=" << media.msid->appData;

        ss << std::endl;

        for (const auto& fmt: media.mediaFormats) {
            ss << "   - format: pt=" << fmt.pt << ", codec=" << fmt.codec << "/"
               << fmt.clockRate;

            if (!fmt.fbTypes.empty())
                ss << ", fb=[" << util::joinStrings(fmt.fbTypes, ",") << "]";

            ss << std::endl;
        }

        if (media.ssrcDescription) {
            for (const auto& ssrc: media.ssrcDescription->ssrcs) {
                ss << "   - ssrc: id=" << ssrc.ssrc << ", msid=" << ssrc.msid << std::endl;
            }
        }

        if (media.iceUfrag && media.icePwd) {
            ss << "   - ice: ufrag=" << *media.iceUfrag << ", pwd=" << *media.icePwd << std::endl;
        }

        auto highestPrio = !media.iceCandidates.empty() ?
                media.highestPriorityICECandidate().priority : 0;

        for (auto i = 0; i < media.iceCandidates.size(); i++) {

            const auto& c = media.iceCandidates[i];
            ss << "   - ice: type=" << c.type << ", addr=" << c.address << ":" << c.port << "/"
               << c.protocol << ", prio=" << c.priority
               << (c.priority == highestPrio ? " [highest]" : "");

            if (i != media.iceCandidates.size()-1)
                ss << std::endl;
        }

        if (mediaIndex != mediaDescriptions.size() - 1) {
            ss << std::endl;
        }
    }

    return ss.str();
}

std::string SDP::bundleLine(unsigned count) {

    std::stringstream ss;
    ss << "a=group:BUNDLE";

    for (unsigned i = 0; i < count; i++)
        ss << " " << i;

    return ss.str();
}


SDP::Direction SDP::directionFromString(const std::string& s) {

    if (s == "recvonly") {
        return Direction::recvonly;
    } else if (s == "sendrecv") {
        return Direction::sendrecv;
    } else if (s == "sendonly") {
        return Direction::sendonly;
    } else if (s == "inactive") {
        return Direction::inactive;
    } else {
        throw std::invalid_argument("SDP::directionFromString: could not parse direction");
    }
}

std::string SDP::directionString(const Direction& d) {

    switch (d) {
        case Direction::recvonly: return "recvonly";
        case Direction::sendrecv: return "sendrecv";
        case Direction::sendonly: return "sendonly";
        case Direction::inactive: return "inactive";
    }

    return "";
}

std::string SDP::typeString(const MediaDescription::Type& t) {

    switch (t) {
        case MediaDescription::Type::audio: return "audio";
        case MediaDescription::Type::video: return "video";
    }

    return "";
}

std::vector<SDP::ICECandidate> SDP::filterCandidatesBySubnet(
    const std::vector<SDP::ICECandidate>& candidates, const net::IPv4Prefix& net) {

    std::vector<ICECandidate> filtered;

    std::copy_if(candidates.begin(), candidates.end(), std::back_inserter(filtered),
        [net](const auto& c) {
        return net::IPv4{c.address}.isInSubnet(net.ip(), net.prefix());
    });

    return filtered;
}

SDP::ICECandidate SDP::highestPriorityICECandidate(
    const std::vector<SDP::ICECandidate>& candidates) {

    return *(std::max_element(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
        return a.priority < b.priority;
    }));
}

#include <catch.h>
#include "proto/sdp.h"
#include "sdp_lines.h"

TEST_CASE("SDP::SessionDescription", "[sdp]") {

    std::vector<std::string> defaultSdpLines = {
        "v=0",
        "o=- 0 0 IN IP4 0.0.0.0",
        "s=-",
        "t=0 0"
    };

    std::vector<std::string> sdpLines1 = {
        "v=0",
        "o=- 95594845933178105 2 IN IP4 127.0.0.1",
        "s=-",
        "t=0 0",
        "a=group:BUNDLE 0",
        "a=extmap-allow-mixed",
        "a=msid-semantic: WMS QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ",
        "m=video 52232 RTP/AVPF 96",
        "a=rtcp:9 IN IP4 0.0.0.0",
        "a=mid:0",
        "a=sendrecv",
        "a=msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8",
        "a=rtcp-mux",
        "a=rtcp-rsize",
        "a=rtpmap:96 VP8/90000",
        "a=rtcp-fb:96 goog-remb",
        "a=rtcp-fb:96 transport-cc",
        "a=ssrc:234879207 cname:ifItV0Qrd2vPYEW9",
        "a=ssrc:234879207 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8"
    };

    std::vector<std::string> sdpLines2 = {
        "v=0",
        "o=- 95594845933178105 2 IN IP4 127.0.0.1",
        "s=-",
        "t=0 0",
        "a=group:BUNDLE 0 1",
        "a=extmap-allow-mixed",
        "a=msid-semantic: WMS QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ",
        "m=video 52232 RTP/AVPF 96",
        "a=mid:0",
        "a=sendrecv",
        "a=rtpmap:96 VP8/90000",
        "a=ssrc:234879207 cname:ifItV0Qrd2vPYEW9",
        "a=ssrc:234879207 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8",
        "m=video 52233 RTP/AVPF 96",
        "a=mid:1",
        "a=sendrecv",
        "a=rtpmap:96 VP8/90000",
        "a=ssrc:234879208 cname:ifItV0Qrd2vPYEW9",
        "a=ssrc:234879208 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8"
    };


    SECTION("initializes an empty session description with default values") {

        SDP::SessionDescription sdp;

        CHECK(sdp.version == 0);
        CHECK(sdp.origin.userName == "-");
        CHECK(sdp.origin.sessionId == 0);
        CHECK(sdp.origin.sessionVersion == 0);
        CHECK(sdp.origin.netType == "IN");
        CHECK(sdp.origin.addrType == "IP4");
        CHECK(sdp.origin.unicastAddr == "0.0.0.0");
        CHECK(sdp.sessionName == "-");
        CHECK(sdp.timing.size() == 1);
        CHECK(sdp.timing[0].start == 0);
        CHECK(sdp.timing[0].stop == 0);
        CHECK(sdp.mediaDescriptions.empty());

        CHECK(sdp.lines() == defaultSdpLines);
    }

    SECTION("parses a session description") {

        SDP::SessionDescription sdp1(sdpLines1);
        // CHECK(sdp1.otherLines.size() == 6);
        CHECK(sdp1.mediaDescriptions.size() == 1);
        CHECK(sdp1.mediaDescriptions[0].direction.has_value());
        CHECK(*sdp1.mediaDescriptions[0].direction == SDP::Direction::sendrecv);
        CHECK(sdp1.mediaDescriptions[0].ssrcDescription->ssrcs[0].ssrc == 234879207);

        SDP::SessionDescription sdp2(sdpLines2);
        // CHECK(sdp2.otherLines.size() == 6);
        CHECK(sdp2.mediaDescriptions.size() == 2);
        CHECK(sdp2.mediaDescriptions[0].direction.has_value());
        CHECK(*sdp2.mediaDescriptions[0].direction == SDP::Direction::sendrecv);
        CHECK(sdp2.mediaDescriptions[0].ssrcDescription->ssrcs[0].ssrc == 234879207);
        CHECK(sdp2.mediaDescriptions[1].direction.has_value());
        CHECK(*sdp2.mediaDescriptions[1].direction == SDP::Direction::sendrecv);
        CHECK(sdp2.mediaDescriptions[1].ssrcDescription->ssrcs[0].ssrc == 234879208);
    }

    SECTION("reconstructs a session description") {

        SDP::SessionDescription sdp1(sdpLines1);
        CHECK(sdp1.lines() == sdpLines1);

        SDP::SessionDescription sdp2(sdpLines2);
        CHECK(sdp2.lines() == sdpLines2);
    }

    SECTION("constructs a session description") {

        SDP::SessionDescription sdp;
        sdp.origin.sessionId = 8203304298658566238;
        sdp.origin.sessionVersion = 2;
        sdp.origin.unicastAddr = "127.0.0.1";

        sdp.otherLines = {
            "a=extmap-allow-mixed",
            "a=msid-semantic: WMS"
        };

        // ability to assemble m-lines tested elsewhere, so just load from test data:
        SDP::MediaDescription m1(test::sdpOfferMLines);
        sdp.mediaDescriptions.push_back(m1);

        CHECK(sdp.lines() == test::sdpOfferLines1);
    }
}

TEST_CASE("SDP::Origin", "[sdp]") {

    std::string oLine = "o=- 95594845933178105 2 IN IP4 127.0.0.1";

    SECTION("parses an origin line") {

        SDP::Origin origin(oLine);
        CHECK(origin.userName == "-");
        CHECK(origin.sessionId == 95594845933178105);
        CHECK(origin.sessionVersion == 2);
        CHECK(origin.netType == "IN");
        CHECK(origin.addrType == "IP4");
        CHECK(origin.unicastAddr == "127.0.0.1");
    }

    SECTION("reconstructs an origin line") {

        SDP::Origin origin(oLine);
        CHECK(origin.line() == oLine);
    }
}

TEST_CASE("SDP::Timing", "[sdp]") {

    std::string tLine1 = "t=0 0";
    std::string tLine2 = "t=438399039338 2389292489248";

    SECTION("parses a timing line") {

        SDP::Timing timing1(tLine1);
        CHECK(timing1.start == 0);
        CHECK(timing1.stop == 0);

        SDP::Timing timing2(tLine2);
        CHECK(timing2.start == 438399039338);
        CHECK(timing2.stop == 2389292489248);
    }

    SECTION("reconstructs a timing line") {

        SDP::Timing timing1(tLine1);
        CHECK(timing1.line() == tLine1);

        SDP::Timing timing2(tLine2);
        CHECK(timing2.line() == tLine2);
    }
}

TEST_CASE("SDP::ConnectionData", "[sdp]") {

    std::string cLine = "c=IN IP4 10.8.50.22";

    SECTION("parses a connection data line") {

        SDP::ConnectionData connectionData(cLine);

        CHECK(connectionData.netType == "IN");
        CHECK(connectionData.addrType == "IP4");
        CHECK(connectionData.connectionAddress == "10.8.50.22");
    }

    SECTION("reconstructs an origin line") {

        SDP::ConnectionData connectionData(cLine);
        CHECK(connectionData.line() == cLine);
    }
}

TEST_CASE("SDP::MediaFormat", "[sdp]") {

    std::vector<std::string> formatLines1 = {
        "a=rtpmap:96 VP8/90000",
        "a=rtcp-fb:96 goog-remb",
        "a=rtcp-fb:96 transport-cc",
        "a=rtcp-fb:96 ccm fir",
        "a=rtcp-fb:96 nack",
        "a=rtcp-fb:96 nack pli"
    };

    std::vector<std::string> formatLines2 = {
        "a=rtpmap:97 rtx/90000",
        "a=fmtp:97 apt=96"
    };

    SECTION("parses a media format") {

        SDP::MediaFormat format1(formatLines1);
        CHECK(format1.pt == 96);
        CHECK(format1.codec == "VP8");
        CHECK(format1.clockRate == 90000);
        CHECK(format1.fbTypes.size() == 5);
        CHECK(format1.fbTypes[0] == "goog-remb");
        CHECK(format1.fbTypes[1] == "transport-cc");
        CHECK(format1.fbTypes[2] == "ccm fir");
        CHECK(format1.fbTypes[3] == "nack");
        CHECK(format1.fbTypes[4] == "nack pli");

        SDP::MediaFormat format2(formatLines2);
        CHECK(format2.pt == 97);
        CHECK(format2.codec == "rtx");
        CHECK(format2.clockRate == 90000);
        CHECK(format2.fmtParams[0] == "apt=96");
    }

    SECTION("reconstructs a media format") {

        SDP::MediaFormat format1(formatLines1);
        auto returnLines1 = format1.lines();

        SDP::MediaFormat format2(formatLines2);
        auto returnLines2 = format2.lines();

        CHECK(formatLines1.size() == returnLines1.size());
        CHECK(formatLines2.size() == returnLines2.size());

        for (auto i = 0; i < formatLines1.size(); i++) {
            CHECK(format1.lines()[i] == formatLines1[i]);
        }

        for (auto i = 0; i < formatLines2.size(); i++) {
            CHECK(format2.lines()[i] == formatLines2[i]);
        }
    }
}

TEST_CASE("SDP::MediaStreamIdentifier", "[sdp]") {

    std::string msidLine1 = "a=msid:- 8229a140-ead3-4a09-b8cb-bf153d979fac";
    std::string msidLine2 = "a=msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8";
    std::string msidLine3 = "a=msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ";

    SECTION("parses a media stream identifier") {

        SDP::MediaStreamIdentifier msid1(msidLine1);
        CHECK(msid1.identifier == "-");
        CHECK(msid1.appData == "8229a140-ead3-4a09-b8cb-bf153d979fac");

        SDP::MediaStreamIdentifier msid2(msidLine2);
        CHECK(msid2.identifier == "QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ");
        CHECK(msid2.appData == "2941816a-39c3-4378-b8c7-725379a042b8");

        SDP::MediaStreamIdentifier msid3(msidLine3);
        CHECK(msid3.identifier == "QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ");
        CHECK(msid3.appData.empty());

    }

    SECTION("reconstructs a media stream identifier") {

        SDP::MediaStreamIdentifier msid1(msidLine1);
        CHECK(msid1.line() == msidLine1);

        SDP::MediaStreamIdentifier msid2(msidLine2);
        CHECK(msid2.line() == msidLine2);

        SDP::MediaStreamIdentifier msid3(msidLine3);
        CHECK(msid3.line() == msidLine3);
    }
}

TEST_CASE("SDP::RTCPConfiguration", "[sdp]") {

    std::string rtcpLine1 = "a=rtcp:9 IN IP4 0.0.0.0";
    std::string rtcpLine2 = "a=rtcp:53020";
    std::string rtcpLine3 = "a=rtcp:53020 IN IP4 126.16.64.4";

    SECTION("parses a media stream identifier") {

        SDP::RTCPConfiguration rtcp1(rtcpLine1);
        CHECK(rtcp1.port == 9);
        CHECK(rtcp1.netType == "IN");
        CHECK(rtcp1.addrType == "IP4");
        CHECK(rtcp1.connectionAddress == "0.0.0.0");

        SDP::RTCPConfiguration rtcp2(rtcpLine2);
        CHECK(rtcp2.port == 53020);
        CHECK(rtcp2.netType.empty());
        CHECK(rtcp2.addrType.empty());
        CHECK(rtcp2.connectionAddress.empty());

        SDP::RTCPConfiguration rtcp3(rtcpLine3);
        CHECK(rtcp3.port == 53020);
        CHECK(rtcp3.netType == "IN");
        CHECK(rtcp3.addrType == "IP4");
        CHECK(rtcp3.connectionAddress == "126.16.64.4");
    }

    SECTION("reconstructs a media stream identifier") {

        SDP::RTCPConfiguration rtcp1(rtcpLine1);
        CHECK(rtcp1.line() == rtcpLine1);

        SDP::RTCPConfiguration rtcp2(rtcpLine2);
        CHECK(rtcp2.line() == rtcpLine2);

        SDP::RTCPConfiguration rtcp3(rtcpLine3);
        CHECK(rtcp3.line() == rtcpLine3);
    }
}

TEST_CASE("SDP::RTPExtension", "[sdp]") {

    std::string extMapLine1 = "a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space";
    std::string extMapLine2 = "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid";
    std::string extMapLine3 = "a=extmap:13 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension";

    SECTION("parses a rtp extension definition") {

        SDP::RTPExtension ext1(extMapLine1);
        CHECK(ext1.identifier == 8);
        CHECK(ext1.uri == "http://www.webrtc.org/experiments/rtp-hdrext/color-space");

        SDP::RTPExtension ext2(extMapLine2);
        CHECK(ext2.identifier == 9);
        CHECK(ext2.uri == "urn:ietf:params:rtp-hdrext:sdes:mid");

        SDP::RTPExtension ext3(extMapLine3);
        CHECK(ext3.identifier == 13);
        CHECK(ext3.uri == "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension");
    }

    SECTION("reconstructs a rtp extension definition") {

        SDP::RTPExtension ext1(extMapLine1);
        CHECK(ext1.line() == extMapLine1);

        SDP::RTPExtension ext2(extMapLine2);
        CHECK(ext2.line() == extMapLine2);

        SDP::RTPExtension ext3(extMapLine3);
        CHECK(ext3.line() == extMapLine3);
    }
}

TEST_CASE("SDP::MediaDescription", "[sdp]") {

    std::vector<std::string> mLines1 = {
        "m=video 52232 RTP/AVPF 96 97 102 122",
        "c=IN IP4 172.16.21.52",
        "a=rtcp:9 IN IP4 0.0.0.0",
        "a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10",
        "a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10",
        "a=ice-ufrag:187l",
        "a=ice-pwd:cRX3nlLJO+9fRF9HyF6tdss7",
        "a=ice-options:trickle",
        "a=mid:0",
        "a=extmap:1 urn:ietf:params:rtp-hdrext:toffset",
        "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
        "a=extmap:3 urn:3gpp:video-orientation",
        "a=extmap:4 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01",
        "a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay",
        "a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type",
        "a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing",
        "a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space",
        "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
        "a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id",
        "a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id",
        "a=extmap:13 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
        "a=sendrecv",
        "a=msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8",
        "a=rtcp-mux",
        "a=rtcp-rsize",
        "a=rtpmap:96 VP8/90000",
        "a=rtcp-fb:96 goog-remb",
        "a=rtcp-fb:96 transport-cc",
        "a=rtcp-fb:96 ccm fir",
        "a=rtcp-fb:96 nack",
        "a=rtcp-fb:96 nack pli",
        "a=rtpmap:97 rtx/90000",
        "a=fmtp:97 apt=96",
        "a=rtpmap:102 H264/90000",
        "a=rtcp-fb:102 goog-remb",
        "a=rtcp-fb:102 transport-cc",
        "a=rtcp-fb:102 ccm fir",
        "a=rtcp-fb:102 nack",
        "a=rtcp-fb:102 nack pli",
        "a=fmtp:102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f",
        "a=rtpmap:122 rtx/90000",
        "a=fmtp:122 apt=102",
        "a=ssrc-group:FID 234879207 2308128374",
        "a=ssrc:234879207 cname:ifItV0Qrd2vPYEW9",
        "a=ssrc:234879207 msid:- QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8",
        "a=ssrc:2308128374 cname:ifItV0Qrd2vPYEW9",
        "a=ssrc:2308128374 msid:- QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8"
    };

    std::vector<std::string> audioMLines = {
        "m=audio 9 RTP/AVPF 111",
        "c=IN IP4 0.0.0.0",
        "a=rtcp:9 IN IP4 0.0.0.0",
        "a=ice-ufrag:JeYY",
        "a=ice-pwd:OkcmKI5LNAr22A9v+5P6yi3F",
        "a=ice-options:trickle",
        "a=mid:1",
        "a=extmap:14 urn:ietf:params:rtp-hdrext:ssrc-audio-level",
        "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
        "a=extmap:4 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01",
        "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
        "a=sendonly",
        "a=msid:- d2305dfd-58af-421e-99fa-22c94d7f6e05",
        "a=rtcp-mux",
        "a=rtpmap:111 opus/48000/2",
        "a=rtcp-fb:111 transport-cc",
        "a=fmtp:111 minptime=10;useinbandfec=1",
        "a=ssrc:1962338720 cname:aGFrTGsMl0bR1Cmz",
        "a=ssrc:1962338720 msid:- d2305dfd-58af-421e-99fa-22c94d7f6e05"
    };


    SECTION("parses a video media description") {

        SDP::MediaDescription m1(mLines1);

        CHECK(m1.type == SDP::MediaDescription::Type::video);
        CHECK(m1.port == 52232);
        CHECK(m1.protocols == "RTP/AVPF");

        CHECK(m1.iceUfrag.has_value());
        CHECK(m1.iceUfrag.value() == "187l");

        CHECK(m1.icePwd.has_value());
        CHECK(m1.icePwd.value() == "cRX3nlLJO+9fRF9HyF6tdss7");

        CHECK(m1.mid.has_value());
        CHECK(m1.mid.value() == "0");

        CHECK(m1.direction.has_value());
        CHECK(m1.direction.value() == SDP::Direction::sendrecv);

        CHECK(m1.msid.has_value());
        CHECK(m1.msid->identifier == "QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ");
        CHECK(m1.msid->appData == "2941816a-39c3-4378-b8c7-725379a042b8");

        // more tests on parsing of ice candidates, formats, ssrc description in other unit tests
        CHECK(m1.iceCandidates.size() == 2);
        CHECK(m1.iceCandidates[0].foundation == "2170762578");
        CHECK(m1.iceCandidates[1].foundation == "3487615394");

        CHECK(m1.mediaFormats.size() == 4);
        CHECK(m1.mediaFormats[0].pt == 96);
        CHECK(m1.mediaFormats[1].pt == 97);
        CHECK(m1.mediaFormats[2].pt == 102);
        CHECK(m1.mediaFormats[3].pt == 122);

        CHECK(m1.ssrcDescription.has_value());
        CHECK(m1.ssrcDescription->ssrcs.size() == 2);
        CHECK(m1.ssrcDescription->ssrcs[0].ssrc == 234879207);
        CHECK(m1.ssrcDescription->ssrcs[1].ssrc == 2308128374);
    }

    SECTION("parses an audio media description") {

        SDP::MediaDescription m{audioMLines};
        CHECK(m.type == SDP::MediaDescription::Type::audio);
        CHECK(m.port == 9);
        CHECK(m.protocols == "RTP/AVPF");

        CHECK(m.iceUfrag.has_value());
        CHECK(m.iceUfrag.value() == "JeYY");

        CHECK(m.icePwd.has_value());
        CHECK(m.icePwd.value() == "OkcmKI5LNAr22A9v+5P6yi3F");

        CHECK(m.mid.has_value());
        CHECK(m.mid.value() == "1");

        CHECK(m.direction.has_value());
        CHECK(m.direction.value() == SDP::Direction::sendonly);

        CHECK(m.msid.has_value());
        CHECK(m.msid->identifier == "-");
        CHECK(m.msid->appData == "d2305dfd-58af-421e-99fa-22c94d7f6e05");

        CHECK(m.mediaFormats.size() == 1);

        CHECK(m.mediaFormats[0].pt == 111);
        CHECK(m.mediaFormats[0].codec == "opus");
        CHECK(m.mediaFormats[0].clockRate == 48000);
        CHECK(m.mediaFormats[0].channels.has_value());
        CHECK(m.mediaFormats[0].channels.value() == 2);

        CHECK(m.mediaFormats[0].fbTypes.size() == 1);
        CHECK(m.mediaFormats[0].fbTypes[0] == "transport-cc");
        CHECK(m.mediaFormats[0].fmtParams.size() == 1);
        CHECK(m.mediaFormats[0].fmtParams[0] == "minptime=10;useinbandfec=1");

        CHECK(m.ssrcDescription.has_value());
        CHECK(m.ssrcDescription->ssrcs.size() == 1);
        CHECK(m.ssrcDescription->ssrcs[0].ssrc == 1962338720);
    }

    SECTION("reconstructs a video media description") {

        SDP::MediaDescription m1(mLines1);
        CHECK(m1.lines() == mLines1);
    }

    SECTION("reconstructs an audio media description") {

        SDP::MediaDescription m{audioMLines};
        CHECK(m.lines() == audioMLines);
    }

    SECTION("constructs a video media description") {

        SDP::MediaDescription m;
        m.type = SDP::MediaDescription::Type::video;
        m.port = 60975;
        m.protocols = "RTP/AVPF";

        m.connectionData = SDP::ConnectionData{"IN", "IP4", "10.8.50.22"};
        m.rtcpConfiguration = SDP::RTCPConfiguration{9, "IN", "IP4", "0.0.0.0"};

        SDP::ICECandidate c1;
        c1.foundation = "2509548753";
        c1.componentId = 1;
        c1.protocol = "udp";
        c1.priority = 2122260224;
        c1.address = "10.8.50.22";
        c1.port = 60975;
        c1.type = "host";
        c1.remainder = "generation 0 network-id 1 network-cost 10";

        SDP::ICECandidate c2;
        c2.foundation = "3675738145";
        c2.componentId = 1;
        c2.protocol = "tcp";
        c2.priority = 1518280448;
        c2.address = "10.8.50.22";
        c2.port = 9;
        c2.type = "host";
        c2.remainder = "tcptype active generation 0 network-id 1 network-cost 10";

        m.iceCandidates = {c1, c2};

        m.iceUfrag = "zpyY";
        m.icePwd = "nKNWiTwEF8JF6Y2wAIG/hH0A";

        m.iceOtherLines = {
            "a=ice-options:trickle"
        };

        m.msid = SDP::MediaStreamIdentifier{"-", "42f41fd3-296c-468a-8bd6-b8c4bf60daa9"};

        m.rtcpOtherLines = {
            "a=rtcp-mux",
            "a=rtcp-rsize"
        };

        m.extensions = {
            {1, "urn:ietf:params:rtp-hdrext:toffset"},
            {2, "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"},
            {3, "urn:3gpp:video-orientation"},
            {4, "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"},
            {5, "http://www.webrtc.org/experiments/rtp-hdrext/playout-delay"},
            {6, "http://www.webrtc.org/experiments/rtp-hdrext/video-content-type"},
            {7, "http://www.webrtc.org/experiments/rtp-hdrext/video-timing"},
            {8, "http://www.webrtc.org/experiments/rtp-hdrext/color-space"},
            {9, "urn:ietf:params:rtp-hdrext:sdes:mid"},
            {10, "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id"},
            {11, "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id"},
            {13, "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension"}
        };

        m.direction = SDP::Direction::sendonly;

        m.mid = "0";

        SDP::MediaFormat m45{45, "AV1", 90000};
        m45.fbTypes = {"goog-remb", "transport-cc", "ccm fir", "nack", "nack pli"};
        m.mediaFormats.push_back(m45);

        SDP::MediaFormat m46{46, "rtx", 90000};
        m46.fmtParams = {"apt=45"};
        m.mediaFormats.push_back(m46);

        m.ssrcDescription = SDP::SSRCDescription{{
             {4071773911, "m2es8fGVVPlmqwQB",  "- 42f41fd3-296c-468a-8bd6-b8c4bf60daa9"},
             {2320959321, "m2es8fGVVPlmqwQB",  "- 42f41fd3-296c-468a-8bd6-b8c4bf60daa9"}
        }};

        CHECK(m.lines() == test::sdpOfferMLines);
    }
}

TEST_CASE("SDP::MediaDescription: highestPriorityICECandidate", "[sdp]") {

    std::vector<std::string> mLines1 = {
        "m=video 52232 RTP/AVPF 96 97 102 122",
        "c=IN IP4 172.16.21.52",
        "a=rtcp:9 IN IP4 0.0.0.0",
        "a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10",
        "a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10"
    };

    SDP::MediaDescription media1(mLines1);

    CHECK(media1.iceCandidates.size() == 2);

    auto c = media1.highestPriorityICECandidate();
    CHECK(c.foundation == "2170762578");
}

TEST_CASE("SDP::SSRCDescription", "[sdp]") {

    std::vector<std::string> singleSSRCLines = {
        "a=ssrc:2829761194 cname:gE2EmxPPUB65XLCe",
        "a=ssrc:2829761194 msid:QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb"
    };

    std::vector<std::string> ssrcGroupLines = {
        "a=ssrc-group:FID 415463049 2829761194",
        "a=ssrc:415463049 cname:gE2EmxPPUB65XLCe",
        "a=ssrc:415463049 msid:QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb",
        "a=ssrc:2829761194 cname:gE2EmxPPUB65XLCe",
        "a=ssrc:2829761194 msid:QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb"
    };

    SECTION("parses a single ssrc") {

        SDP::SSRCDescription singleSSRC(singleSSRCLines);
        CHECK(singleSSRC.ssrcs.size() == 1);
        CHECK(singleSSRC.ssrcs[0].ssrc == 2829761194);
        CHECK(singleSSRC.ssrcs[0].cname == "gE2EmxPPUB65XLCe");
        CHECK(singleSSRC.ssrcs[0].msid == "QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb");
    }

    SECTION("parses an ssrc group") {

        SDP::SSRCDescription ssrcGroup(ssrcGroupLines);
        CHECK(ssrcGroup.ssrcs.size() == 2);
        CHECK(ssrcGroup.ssrcs[0].ssrc == 415463049);
        CHECK(ssrcGroup.ssrcs[0].cname == "gE2EmxPPUB65XLCe");
        CHECK(ssrcGroup.ssrcs[0].msid == "QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb");
        CHECK(ssrcGroup.ssrcs[1].ssrc == 2829761194);
        CHECK(ssrcGroup.ssrcs[1].cname == "gE2EmxPPUB65XLCe");
        CHECK(ssrcGroup.ssrcs[1].msid == "QVyETJ2kHFWQLjTapNKwb2jRRMxb7n4SUhJM 00317b5b-a623-47d7-ae78-6fe88f54b7cb");
    }

    SECTION("reconstructs a single ssrc") {

        SDP::SSRCDescription singleSSRC(singleSSRCLines);
        auto lines = singleSSRC.lines();
        CHECK(lines.size() == 2);
        CHECK(lines == singleSSRCLines);
    }

    SECTION("reconstructs an ssrc group") {

        SDP::SSRCDescription ssrcGroup(ssrcGroupLines);
        auto lines = ssrcGroup.lines();
        CHECK(lines.size() == 5);
        CHECK(lines == ssrcGroupLines);
    }
}

TEST_CASE("SDP::ICECandidate", "[sdp]") {

    std::string candidateLine1 = "a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10";
    std::string candidateLine2 = "a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10";

    SECTION("parses an ice candidate") {

        SDP::ICECandidate ice1(candidateLine1);
        CHECK(ice1.foundation == "2170762578");
        CHECK(ice1.componentId == 1);
        CHECK(ice1.protocol == "udp");
        CHECK(ice1.priority == 2122194687);
        CHECK(ice1.address == "172.16.21.51");
        CHECK(ice1.port == 54903);
        CHECK(ice1.type == "host");
        CHECK(ice1.remainder == "generation 0 network-id 1 network-cost 10");

        SDP::ICECandidate ice2(candidateLine2);
        CHECK(ice2.foundation == "3487615394");
        CHECK(ice2.componentId == 1);
        CHECK(ice2.protocol == "tcp");
        CHECK(ice2.priority == 1518214911);
        CHECK(ice2.address == "172.16.21.51");
        CHECK(ice2.port == 9);
        CHECK(ice2.type == "host");
        CHECK(ice2.remainder == "tcptype active generation 0 network-id 1 network-cost 10");
    }

    SECTION("reconstructs an ice candidate") {

        SDP::ICECandidate ice1(candidateLine1);
        SDP::ICECandidate ice2(candidateLine2);
        CHECK(ice1.line() == candidateLine1);
        CHECK(ice2.line() == candidateLine2);
    }
}

TEST_CASE("SDP::bundleLine", "[sdp]") {

    CHECK(SDP::bundleLine(0) == "a=group:BUNDLE");
    CHECK(SDP::bundleLine(1) == "a=group:BUNDLE 0");
    CHECK(SDP::bundleLine(2) == "a=group:BUNDLE 0 1");
    CHECK(SDP::bundleLine(3) == "a=group:BUNDLE 0 1 2");
}

TEST_CASE("SDP::directionFromString", "[sdp]") {

    CHECK(SDP::directionFromString("recvonly") == SDP::Direction::recvonly);
    CHECK(SDP::directionFromString("sendrecv") == SDP::Direction::sendrecv);
    CHECK(SDP::directionFromString("sendonly") == SDP::Direction::sendonly);
    CHECK(SDP::directionFromString("inactive") == SDP::Direction::inactive);
}

TEST_CASE("SDP::directionString", "[sdp]") {

    CHECK(SDP::directionString(SDP::Direction::recvonly) == "recvonly");
    CHECK(SDP::directionString(SDP::Direction::sendrecv) == "sendrecv");
    CHECK(SDP::directionString(SDP::Direction::sendonly) == "sendonly");
    CHECK(SDP::directionString(SDP::Direction::inactive) == "inactive");
}

TEST_CASE("SDP::filterCandidatesBySubnet", "[sdp]") {

    std::string candidateLine1 = "a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10";
    std::string candidateLine2 = "a=candidate:2170762579 1 udp 2122194600 10.0.0.2 54922 typ host generation 0 network-id 1 network-cost 10";

    std::vector candidates = {SDP::ICECandidate(candidateLine1), SDP::ICECandidate(candidateLine2)};
    auto filtered = SDP::filterCandidatesBySubnet(candidates, net::IPv4Prefix{"10.0.0.0", 24});

    CHECK(filtered.size() == 1);
    CHECK(std::find_if(filtered.begin(), filtered.end(), [&](const SDP::ICECandidate& c) {
        return c.address == "10.0.0.2";
    }) != filtered.end());
}

TEST_CASE("SDP::highestPriorityICECandidate", "[sdp]") {

    std::string candidateLine1 = "a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10";
    std::string candidateLine2 = "a=candidate:2170762579 1 udp 2122194600 10.0.0.2 54922 typ host generation 0 network-id 1 network-cost 10";

    std::vector candidates = {SDP::ICECandidate(candidateLine1), SDP::ICECandidate(candidateLine2)};

    auto highest = SDP::highestPriorityICECandidate(candidates);

    CHECK(highest.address == "172.16.21.51");
}
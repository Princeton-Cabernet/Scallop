#include <catch.h>

#include <session.h>
#include <stream.h>

#include "misc_data.h"
#include "net/net.h"

using namespace p4sfu;

static SDP::SessionDescription offer1{std::vector<std::string> {
    "v=0",
    "o=- 7055464300269812701 2 IN IP4 127.0.0.1",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61575 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61575 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYY",
    "a=ice-pwd:OkcmKI5LNAr22A9v+5P6yi3F",
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
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=sendonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 ccm fir",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
    "a=ssrc-group:FID 207783906 2917303371",
    "a=ssrc:207783906 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:207783906 msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=ssrc:2917303371 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:2917303371 msid:- 91d54343-cce3-4423-88ce-3b294b563feb"
}};

static SDP::SessionDescription answer1{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 1.2.3.4",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 3202 RTP/AVPF 45 46",
    "c=IN IP4 1.2.3.4",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:0 1 udp 1 1.2.3.4 3202 typ host generation 0 network-id 1 network-cost 10",
    "a=ice-ufrag:7Ad/",
    "a=ice-pwd:UKZe/aYNEouGzQUhChnKGiIS",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45"
}};

static SDP::SessionDescription offer2{std::vector<std::string> {
    "v=0",
    "o=- 7055464300269812701 3 IN IP4 127.0.0.1",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0 1",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61575 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61575 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYY",
    "a=ice-pwd:OkcmKI5LNAr22A9v+5P6yi3F",
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
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=sendonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 ccm fir",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
    "a=ssrc-group:FID 207783906 2917303371",
    "a=ssrc:207783906 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:207783906 msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=ssrc:2917303371 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:2917303371 msid:- 91d54343-cce3-4423-88ce-3b294b563feb",

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
}};

static SDP::SessionDescription answer2{std::vector<std::string>{
    "v=0",
    "o=- 0 0 IN IP4 1.2.3.4",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 3202 RTP/AVPF 45 46",
    "c=IN IP4 1.2.3.4",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:0 1 udp 1 1.2.3.4 3202 typ host generation 0 network-id 1 network-cost 10",
    "a=ice-ufrag:7Ad/",
    "a=ice-pwd:UKZe/aYNEouGzQUhChnKGiIS",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",

    "m=audio 3202 RTP/AVPF 111",
    "c=IN IP4 1.2.3.4",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:0 1 udp 1 1.2.3.4 3202 typ host generation 0 network-id 1 network-cost 10",
    "a=ice-ufrag:7Ad/",
    "a=ice-pwd:UKZe/aYNEouGzQUhChnKGiIS",
    "a=ice-options:trickle",
    "a=mid:1",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:14 urn:ietf:params:rtp-hdrext:ssrc-audio-level",
    "a=recvonly",
    "a=rtcp-mux",
    "a=rtpmap:111 opus/48000/2",
    "a=rtcp-fb:111 goog-remb",
    "a=fmtp:111 minptime=10;useinbandfec=1"
}};

static SDP::SessionDescription offer3{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 1.2.3.4",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 3202 RTP/AVPF 45 46",
    "c=IN IP4 1.2.3.4",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:0 1 udp 1 1.2.3.4 3202 typ host generation 0 network-id 1 network-cost 10",
    "a=ice-ufrag:7Ad/",
    "a=ice-pwd:UKZe/aYNEouGzQUhChnKGiIS",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=sendonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
    "a=ssrc-group:FID 207783906 2917303371",
    "a=ssrc:207783906 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:207783906 msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=ssrc:2917303371 cname:aGFrTGsMl0bR1Cmz",
    "a=ssrc:2917303371 msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
}};

static SDP::SessionDescription answer3{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 172.16.21.11",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61576 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61576 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYZ",
    "a=ice-pwd:OkcmKI5LNAr22A934fP6yi3F",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
}};

static SDP::SessionDescription answer4{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 172.16.21.11",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61577 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61577 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYZ",
    "a=ice-pwd:OkcmKI5LNAr22A934fP6yi3F",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
}};

static SDP::SessionDescription offer4{std::vector<std::string> {
    "v=0",
    "o=- 7055464300269812701 2 IN IP4 127.0.0.1",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61576 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61576 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYD",
    "a=ice-pwd:OkcmKI5LNAr22A9v+5P6yi3D",
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
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=sendonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563fd",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 ccm fir",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
    "a=ssrc-group:FID 207783903 2917303373",
    "a=ssrc:207783903 cname:aGFrTGsMl0bR1CmZ",
    "a=ssrc:207783903 msid:- 91d54343-cce3-4423-88ce-3b294b563ffd",
    "a=ssrc:2917303373 cname:aGFrTGsMl0bR1CmZ",
    "a=ssrc:2917303373 msid:- 91d54343-cce3-4423-88ce-3b294b563fd"
}};

static SDP::SessionDescription answer5{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 172.16.21.11",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61581 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61581 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:Jefj",
    "a=ice-pwd:OkcmKI5LNAr22A934fP6yi33",
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
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
}};

static SDP::SessionDescription answer6{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 172.16.21.11",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61592 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61592 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JefH",
    "a=ice-pwd:OkcmKI5LNAr22A934fP6yi36",
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
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",
}};

static SDP::SessionDescription answer7{std::vector<std::string> {
    "v=0",
    "o=- 0 0 IN IP4 172.16.21.11",
    "s=-",
    "t=0 0",
    "a=group:BUNDLE 0",
    "a=extmap-allow-mixed",
    "a=msid-semantic: WMS",

    "m=video 61576 RTP/AVPF 45 46",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=candidate:1377782005 1 udp 2122260224 172.16.21.11 61576 typ host generation 0 network-id 1",
    "a=candidate:2897583201 1 tcp 1518280447 172.16.21.11 9 typ host tcptype active generation 0 network-id 1",
    "a=ice-ufrag:JeYZ",
    "a=ice-pwd:OkcmKI5LNAr22A934fP6yi3F",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:12 https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension",
    "a=recvonly",
    "a=msid:- 91d54343-cce3-4423-88ce-3b294b563feb",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 goog-remb",
    "a=rtcp-fb:45 nack",
    "a=rtcp-fb:45 nack pli",
    "a=rtcp-fb:45 ccm fir",
    "a=fmtp:45 level-idx=5;profile=0;tier=0",
    "a=rtpmap:46 rtx/90000",
    "a=fmtp:46 apt=45",

    "m=audio 61576 RTP/AVPF 111",
    "c=IN IP4 172.16.21.11",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=ice-ufrag:7Ad/",
    "a=ice-pwd:UKZe/aYNEouGzQUhChnKGiIS",
    "a=ice-options:trickle",
    "a=mid:1",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=extmap:9 urn:ietf:params:rtp-hdrext:sdes:mid",
    "a=extmap:14 urn:ietf:params:rtp-hdrext:ssrc-audio-level",
    "a=recvonly",
    "a=rtcp-mux",
    "a=rtpmap:111 opus/48000/2",
    "a=rtcp-fb:111 goog-remb",
    "a=fmtp:111 minptime=10;useinbandfec=1"
}};


TEST_CASE("multi-peer-conn signaling test 1", "[session]") {

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session &session, unsigned participantId,
            const Stream &stream) {
        switch (stream.description().direction.value()) {
            case SDP::Direction::sendonly:
                CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                sendStreams.push_back(stream);
                break;
            case SDP::Direction::recvonly:
                CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(
                        stream.dataPlaneConfig()));
                CHECK(std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                receiveStreams.push_back(stream);
                break;
            default:
                FAIL("stream is neither sendonly nor recvonly");
        }
    });

    // - a session is created
    // - participant 1 joins

    SDP::SessionDescription answerForP1, offerForP1;
    auto p1 = s.addParticipant("10.16.82.226", 1);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    // - participant 1 shares video

    CHECK_NOTHROW(s.addOffer(p1, offer1));
    CHECK(s.signalingActive());
    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.empty());
    CHECK_FALSE(s.signalingActive());

    CHECK(answerForP1.lines() == answer1.lines());

    // - participant 1 shares audio

    CHECK_NOTHROW(s.addOffer(p1, offer2));
    CHECK(s.signalingActive());
    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.empty());
    CHECK_FALSE(s.signalingActive());

    CHECK(answerForP1.lines() == answer2.lines());

    const auto& sendStream1 = sendStreams[0].dataPlaneConfig<Stream::SendStreamConfig>();
    CHECK(sendStream1.mainSSRC == 207783906);
    CHECK(sendStream1.rtxSSRC == 2917303371);
    CHECK(sendStream1.addr.ip() == net::IPv4{"172.16.21.11"});
    CHECK(sendStream1.addr.port() == 61575);
    CHECK(sendStream1.iceUfrag == "JeYY");
    CHECK(sendStream1.icePwd == "OkcmKI5LNAr22A9v+5P6yi3F");

    const auto& sendStream2 = sendStreams[1].dataPlaneConfig<Stream::SendStreamConfig>();
    CHECK(sendStream2.mainSSRC == 1962338720);
    CHECK(sendStream2.rtxSSRC == 0);
    CHECK(sendStream2.addr.ip() == net::IPv4{"172.16.21.11"});
    CHECK(sendStream2.addr.port() == 61575);
    CHECK(sendStream2.iceUfrag == "JeYY");
    CHECK(sendStream2.icePwd == "OkcmKI5LNAr22A9v+5P6yi3F");
}

TEST_CASE("multi-peer-conn signaling test 2", "[session]") {

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session &session, unsigned participantId,
            const Stream &stream) {
        switch (stream.description().direction.value()) {
            case SDP::Direction::sendonly:
                CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                sendStreams.push_back(stream);
                break;
            case SDP::Direction::recvonly:
                CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(
                        stream.dataPlaneConfig()));
                CHECK(std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                receiveStreams.push_back(stream);
                break;
            default:
                FAIL("stream is neither sendonly nor recvonly");
        }
    });

    // - a session is created
    // - participant 1 joins
    // - participant 2 joins
    // - participant 1 starts video

    auto p1 = s.addParticipant("10.16.82.226", 1);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    auto p2 = s.addParticipant("10.16.82.227", 2);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    SDP::SessionDescription offerForP2, answerForP1;

    CHECK_NOTHROW(s.addOffer(p1, offer1));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(offerForP2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);
    CHECK(offerForP2.lines() == offer3.lines());

    CHECK_NOTHROW(s.addAnswer(p2, answer3));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK_THROWS(s.participantInState(Participant::SignalingState::gotOffer));
    CHECK(answerForP1.lines() == answer1.lines());

    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 1);
}

TEST_CASE("multi-peer-conn signaling test 3", "[session]") {

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session &session, unsigned participantId,
            const Stream &stream) {
        switch (stream.description().direction.value()) {
            case SDP::Direction::sendonly:
                CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                sendStreams.push_back(stream);
                break;
            case SDP::Direction::recvonly:
                CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(
                        stream.dataPlaneConfig()));
                CHECK(std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                receiveStreams.push_back(stream);
                break;
            default:
                FAIL("stream is neither sendonly nor recvonly");
        }
    });

    // - a session is created
    // - participant 1 joins
    // - participant 2 joins
    // - participant 3 joins
    // - participant 1 starts video

    auto p1 = s.addParticipant("10.16.82.226", 1);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    auto p2 = s.addParticipant("10.16.82.227", 2);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    auto p3 = s.addParticipant("10.16.82.228", 3);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    SDP::SessionDescription offerForP2, offerForP3, answerForP1;

    CHECK_NOTHROW(s.addOffer(p1, offer1));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(offerForP2 = s.getOffer(p2));
    CHECK_NOTHROW(offerForP3 = s.getOffer(p3));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[p3].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK(offerForP2.lines() == offer3.lines());
    CHECK(offerForP3.lines() == offer3.lines());

    CHECK_NOTHROW(s.addAnswer(p2, answer3));
    CHECK_NOTHROW(s.addAnswer(p3, answer4));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK_THROWS(s.participantInState(Participant::SignalingState::gotOffer));

    CHECK(answerForP1.lines() == answer1.lines());

    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 2);

    // - participant 2 starts video

    SDP::SessionDescription offer2ForP3, offer2ForP1, answerForP2;

    CHECK_NOTHROW(s.addOffer(p2, offer4));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p2);
    CHECK(sendStreams.size() == 2);

    // should throw a logic error when trying to start a new signaling transaction
    CHECK_THROWS_AS(s.addOffer(p1, offer1), std::logic_error);

    CHECK_NOTHROW(offer2ForP1 = s.getOffer(p1));
    CHECK_NOTHROW(offer2ForP3 = s.getOffer(p3));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p3].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p2);

    CHECK(offer2ForP1.mediaDescriptions.size() == 1);
    CHECK(offer2ForP1.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offer2ForP1.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(offer2ForP1.mediaDescriptions[0].ssrcDescription.has_value());
    CHECK(offer2ForP1.mediaDescriptions[0].ssrcDescription->ssrcs.size() == 2);
    CHECK(offer2ForP1.mediaDescriptions[0].ssrcDescription->ssrcs[0].ssrc == 207783903);
    CHECK(offer2ForP1.mediaDescriptions[0].ssrcDescription->ssrcs[1].ssrc == 2917303373);

    CHECK(offer2ForP3.mediaDescriptions.size() == 1);
    CHECK(offer2ForP3.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offer2ForP3.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(offer2ForP3.mediaDescriptions[0].ssrcDescription.has_value());
    CHECK(offer2ForP3.mediaDescriptions[0].ssrcDescription->ssrcs.size() == 2);
    CHECK(offer2ForP3.mediaDescriptions[0].ssrcDescription->ssrcs[0].ssrc == 207783903);
    CHECK(offer2ForP3.mediaDescriptions[0].ssrcDescription->ssrcs[1].ssrc == 2917303373);

    CHECK_NOTHROW(s.addAnswer(p1, answer5));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p3].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p2);
    CHECK(receiveStreams.size() == 3);

    CHECK_NOTHROW(s.addAnswer(p3, answer6));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p2);
    CHECK(receiveStreams.size() == 4);

    CHECK_NOTHROW(answerForP2 = s.getAnswer(p2));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p3].signalingState() == Participant::SignalingState::stable);

    CHECK(answerForP2.mediaDescriptions.size() == 1);
    CHECK(answerForP2.mediaDescriptions[0].direction == SDP::Direction::recvonly);
    CHECK(answerForP2.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(answerForP2.mediaDescriptions[0].mid == "0");
}

TEST_CASE("multi-peer-conn signaling test 4", "[session]") {

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session &session, unsigned participantId,
            const Stream &stream) {
        switch (stream.description().direction.value()) {
            case SDP::Direction::sendonly:
                CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                sendStreams.push_back(stream);
                break;
            case SDP::Direction::recvonly:
                CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(
                        stream.dataPlaneConfig()));
                CHECK(std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                receiveStreams.push_back(stream);
                break;
            default:
                FAIL("stream is neither sendonly nor recvonly");
        }
    });

    // - a session is created
    // - participant 1 joins
    // - participant 2 joins

    auto p1 = s.addParticipant("10.16.82.226", 1);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    auto p2 = s.addParticipant("10.16.82.227", 2);
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    // - participant 1 starts video

    SDP::SessionDescription offerForP2, answerForP1;

    CHECK_NOTHROW(s.addOffer(p1, offer1));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(offerForP2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK(offerForP2.lines() == offer3.lines());
    CHECK_NOTHROW(s.addAnswer(p2, answer3));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK_THROWS(s.participantInState(Participant::SignalingState::gotOffer));

    CHECK(answerForP1.lines() == answer1.lines());

    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 1);

    // - participant 1 starts audio

    SDP::SessionDescription offer2ForP2, answer2ForP1;

    CHECK_NOTHROW(s.addOffer(p1, offer2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(offer2ForP2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK(offer2ForP2.mediaDescriptions.size() == 2);
    CHECK(offer2ForP2.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offer2ForP2.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(offer2ForP2.mediaDescriptions[1].direction == SDP::Direction::sendonly);
    CHECK(offer2ForP2.mediaDescriptions[1].type == SDP::MediaDescription::Type::audio);

    CHECK_NOTHROW(s.addAnswer(p2, answer7));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(s.participantInState(Participant::SignalingState::gotOffer) == p1);

    CHECK_NOTHROW(answer2ForP1 = s.getAnswer(p1));

    CHECK(answer2ForP1.mediaDescriptions.size() == 2);
    CHECK(answer2ForP1.mediaDescriptions[0].direction == SDP::Direction::recvonly);
    CHECK(answer2ForP1.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(answer2ForP1.mediaDescriptions[1].direction == SDP::Direction::recvonly);
    CHECK(answer2ForP1.mediaDescriptions[1].type == SDP::MediaDescription::Type::audio);

    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);

    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 2);
}

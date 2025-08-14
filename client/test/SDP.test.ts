import { SDP } from "../src/SDP";

const testOffer = 
"v=0\r\no=- 95594845933178105 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\na=group:BUNDLE 0\r\na=extmap-allow-mixed\r\na=msid-semantic: WMS QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ\r\n" +
"m=video 52232 RTP/AVPF 96 97 102 122\r\n" +
"c=IN IP4 172.16.21.52\r\na=rtcp:9 IN IP4 0.0.0.0\r\n" +
"a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10\r\n" +
"a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10\r\n" + 
"a=ice-ufrag:187l\r\na=ice-pwd:cRX3nlLJO+9fRF9HyF6tdss7\r\na=ice-options:trickle\r\na=mid:0\r\n" +
"a=extmap:1 urn:ietf:params:rtp-hdrext:toffset\r\na=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\na=extmap:3 urn:3gpp:video-orientation\r\na=extmap:4 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\na=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\na=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\n" +
"a=sendrecv\r\na=msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8\r\na=rtcp-mux\r\na=rtcp-rsize\r\n" +
"a=rtpmap:96 VP8/90000\r\na=rtcp-fb:96 goog-remb\r\na=rtcp-fb:96 transport-cc\r\na=rtcp-fb:96 ccm fir\r\na=rtcp-fb:96 nack\r\na=rtcp-fb:96 nack pli\r\na=rtpmap:97 rtx/90000\r\na=fmtp:97 apt=96\r\n" +
"a=rtpmap:102 H264/90000\r\na=rtcp-fb:102 goog-remb\r\na=rtcp-fb:102 transport-cc\r\na=rtcp-fb:102 ccm fir\r\na=rtcp-fb:102 nack\r\na=rtcp-fb:102 nack pli\r\na=fmtp:102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\na=rtpmap:122 rtx/90000\r\na=fmtp:122 apt=102\r\n" +
"a=ssrc-group:FID 234879207 2308128374\r\na=ssrc:234879207 cname:ifItV0Qrd2vPYEW9\r\na=ssrc:234879207 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8\r\na=ssrc:2308128374 cname:ifItV0Qrd2vPYEW9\r\na=ssrc:2308128374 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8\r\n";


describe("SDP.SessionDescription", () => {

    it("should not throw when given a valid sdp string", () => {

        expect(() => new SDP.SessionDescription(testOffer)).not.toThrow();
    });

    it("should parse the header and media sections", () => {
        
        const sdp = new SDP.SessionDescription(testOffer);
        expect(sdp.header).toBeDefined();
        expect(sdp.mediaDescriptions).toHaveLength(1);
        expect(sdp.mediaDescriptions[0]).toBeDefined();
    });

    it("returns the SDP lines", () => {
        
        const linesInCount = testOffer.split(/\r\n/).filter((line) => line.length > 0).length;
        const sdp = new SDP.SessionDescription(testOffer);

        expect(sdp.lines().length).toBe(linesInCount);
    });
 
});

const mLines1: string[] = [
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
    "a=ssrc:234879207 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8",
    "a=ssrc:2308128374 cname:ifItV0Qrd2vPYEW9",
    "a=ssrc:2308128374 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8"
];

const mLines2: string[] = [
    "m=video 9 RTP/AVPF 45",
    "c=IN IP4 0.0.0.0",
    "a=rtcp:9 IN IP4 0.0.0.0",
    "a=ice-ufrag:irmY",
    "a=ice-pwd:zs4dceIXW/KCuOylT6tFNYY6",
    "a=ice-options:trickle",
    "a=mid:0",
    "a=extmap:1 urn:ietf:params:rtp-hdrext:toffset",
    "a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time",
    "a=sendrecv",
    "a=msid:- 0e2a9974-015b-4f1f-a9d3-9d46cb00eeb9",
    "a=rtcp-mux",
    "a=rtcp-rsize",
    "a=rtpmap:45 AV1/90000",
    "a=rtcp-fb:45 transport-cc",
    "a=rtcp-fb:45 ccm fir",
    "a=rtcp-fb:45 nack",
    "a=ssrc:1381270912 cname:DxW+cz0j3meHMiHM",
    "a=ssrc:1381270912 msid:- 0e2a9974-015b-4f1f-a9d3-9d46cb00eeb9"
];

describe("SDP.MediaDescription", () => {

    const m = new SDP.MediaDescription(mLines1);

    it("parses an m-line", () => {

        expect(m.mLine).toBeDefined();
        expect(m.mLine!.type).toBe("video");
        expect(m.mLine!.port).toBe(52232);
        expect(m.mLine!.protocols).toBe("RTP/AVPF");
        expect(m.mLine!.payloadTypes).toHaveLength(4);
        expect(m.mLine!.payloadTypes).toContain(96);
        expect(m.mLine!.payloadTypes).toContain(97);
        expect(m.mLine!.payloadTypes).toContain(102);
        expect(m.mLine!.payloadTypes).toContain(122);
    });

    it("reconstructs an m-line", () => {

        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()[0]).toBe("m=video 52232 RTP/AVPF 96 97 102 122");
    })

    it("reconstructs an m-line after changes", () => {

        expect(m.mLine).toBeDefined();
        m.mLine!.payloadTypes = m.mLine!.payloadTypes.filter((pt: number) => pt != 122);
        expect(m.lines()[0]).toBe("m=video 52232 RTP/AVPF 96 97 102");
    });

    it("parses ICE candidates", () => {

        expect(m.iceCandidates).toBeDefined();
        expect(m.iceCandidates).toHaveLength(2);

        // a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10
        expect(m.iceCandidates[0].foundation).toBe("2170762578");
        expect(m.iceCandidates[0].componentId).toBe(1);
        expect(m.iceCandidates[0].protocol).toBe("udp");
        expect(m.iceCandidates[0].priority).toBe(2122194687);
        expect(m.iceCandidates[0].address).toBe("172.16.21.51");
        expect(m.iceCandidates[0].port).toBe(54903);
        expect(m.iceCandidates[0].type).toBe("host");
        expect(m.iceCandidates[0].otherAttributes)
            .toBe("generation 0 network-id 1 network-cost 10");

        // a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10
        expect(m.iceCandidates[1].foundation).toBe("3487615394");
        expect(m.iceCandidates[1].componentId).toBe(1);
        expect(m.iceCandidates[1].protocol).toBe("tcp");
        expect(m.iceCandidates[1].priority).toBe(1518214911);
        expect(m.iceCandidates[1].address).toBe("172.16.21.51");
        expect(m.iceCandidates[1].port).toBe(9);
        expect(m.iceCandidates[1].type).toBe("host");
        expect(m.iceCandidates[1].otherAttributes)
            .toBe("tcptype active generation 0 network-id 1 network-cost 10");
    });

    it("reconstructs ICE candidates", () => {

        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()).toContain("a=candidate:2170762578 1 udp 2122194687 172.16.21.51 54903 typ host generation 0 network-id 1 network-cost 10");
        expect(m.lines()).toContain("a=candidate:3487615394 1 tcp 1518214911 172.16.21.51 9 typ host tcptype active generation 0 network-id 1 network-cost 10");
    });

    it ("parses ICE ufrag and pwd", () => {

        expect(m.iceUfrag).toBeDefined();
        expect(m.icePwd).toBeDefined();
        expect(m.iceUfrag).toBe("187l");
        expect(m.icePwd).toBe("cRX3nlLJO+9fRF9HyF6tdss7");
    });

    it("reconstructs ice-ufrag and ice-pwd attributes", () => {

        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()).toContain("a=ice-ufrag:187l");
        expect(m.lines()).toContain("a=ice-pwd:cRX3nlLJO+9fRF9HyF6tdss7");
    });

    it("parses the direction", () => {

        expect(m.direction).toBeDefined();
        expect(m.direction).toBe("sendrecv");
    });

    it("reconstructs the direction", () => {

        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()).toContain("a=sendrecv");
    });

    it ("parses the mid", () => {
        expect(m.mid).toBeDefined();
        expect(m.mid).toBe("0");
    })

    it ("reconstructs the mid", () => {
        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()).toContain("a=mid:0");
    });

    it("parses an SSRC group", () => {

        expect(m.ssrcGroup).toBeDefined();
        expect(m.ssrcGroup!.ssrcs).toBeDefined();
        expect(m.ssrcGroup!.ssrcs).toHaveLength(2);

        expect(m.ssrcGroup!.ssrcs[0].ssrc).toBe(234879207);
        expect(m.ssrcGroup!.ssrcs[0].cname).toBeDefined();
        expect(m.ssrcGroup!.ssrcs[0].cname).toBe("ifItV0Qrd2vPYEW9");
        expect(m.ssrcGroup!.ssrcs[0].msid).toBeDefined();
        expect(m.ssrcGroup!.ssrcs[0].msid).toBe("QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8");
        
        expect(m.ssrcGroup!.ssrcs[1].ssrc).toBe(2308128374);
        expect(m.ssrcGroup!.ssrcs[1].cname).toBeDefined();
        expect(m.ssrcGroup!.ssrcs[1].cname).toBe("ifItV0Qrd2vPYEW9");
        expect(m.ssrcGroup!.ssrcs[1].msid).toBeDefined();
        expect(m.ssrcGroup!.ssrcs[1].msid).toBe("QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8");
    });

    it("reconstructs an SSRC group", () => {

        expect(m.lines()).toHaveLength(mLines1.length);
        expect(m.lines()).toContain("a=ssrc-group:FID 234879207 2308128374");
        expect(m.lines()).toContain("a=ssrc:234879207 cname:ifItV0Qrd2vPYEW9");
        expect(m.lines()).toContain("a=ssrc:234879207 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8");
        expect(m.lines()).toContain("a=ssrc:2308128374 cname:ifItV0Qrd2vPYEW9");
        expect(m.lines()).toContain("a=ssrc:2308128374 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8");
    });

    it("reconstructs an SSRC group after changes", () => {

        m.ssrcGroup!.ssrcs[0].ssrc = 1;
        expect(m.lines()).toContain("a=ssrc-group:FID 1 2308128374");
        expect(m.lines()).toContain("a=ssrc:1 cname:ifItV0Qrd2vPYEW9");
        expect(m.lines()).toContain("a=ssrc:1 msid:QUexxpKtuUKfa7QzqEnj3lvibed0yqJqDtpZ 2941816a-39c3-4378-b8c7-725379a042b8");
    });

    const m2 = new SDP.MediaDescription(mLines2);

    it("parses a single SSRC", () => {

        expect(m2.ssrcGroup).toBeUndefined();
        expect(m2.ssrc).toBeDefined();
        expect(m2.ssrc!.ssrc).toBe(1381270912);
        expect(m2.ssrc!.cname).toBe("DxW+cz0j3meHMiHM");
        expect(m2.ssrc!.msid).toBe("- 0e2a9974-015b-4f1f-a9d3-9d46cb00eeb9");
    });

    it("reconstructs a single SSRC after changes", () => {

        expect(m2.ssrcGroup).toBeUndefined();
        expect(m2.ssrc).toBeDefined();
        m2.ssrc!.ssrc = 4;
        expect(m2.lines()).toContain("a=ssrc:4 cname:DxW+cz0j3meHMiHM");
        expect(m2.lines()).toContain("a=ssrc:4 msid:- 0e2a9974-015b-4f1f-a9d3-9d46cb00eeb9");
    });
});

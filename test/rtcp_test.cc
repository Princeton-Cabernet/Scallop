
#include <catch.h>
#include <arpa/inet.h>

#include "proto/rtcp.h"
#include "rtp_rtcp_packets.h"

#include <iostream>

TEST_CASE("rtcp: can be parsed from a packet buffer", "[rtcp]") {

    auto* rtcp = reinterpret_cast<const rtcp::hdr*>(test::rtp_sr_buf);

    // rtcp common header:
    CHECK(rtcp->version() == 2);
    CHECK(rtcp->padding() == 0);
    CHECK(rtcp->recep_rep_count() == 0);
    CHECK(rtcp->pt == 200);
    CHECK(rtcp->sender_ssrc == ntohl(16778241));

    // sender report
    CHECK(rtcp->data.sr.ntp_ts_msw == ntohl(3841332586));
    CHECK(rtcp->data.sr.ntp_ts_lsw == ntohl(1674737310));
    CHECK(rtcp->data.sr.rtp_ts == ntohl(4164197538));
    CHECK(rtcp->data.sr.sender_pkt_count == ntohl(102777));
    CHECK(rtcp->data.sr.sender_byte_count == ntohl(119018945));
}

TEST_CASE("rtcp: parses receiver reports", "[rtcp]") {

    const auto* rtcp = reinterpret_cast<const rtcp::hdr*>(test::rtcp_rr_buf);

    // rtcp common header:
    CHECK(rtcp->version() == 2);
    CHECK(rtcp->padding() == 0);
    CHECK(rtcp->recep_rep_count() == 2);
    CHECK(rtcp->pt == 201);
    CHECK(rtcp->sender_ssrc == ntohl(3128394281));

    // 1st receiver report:
    CHECK(ntohl(rtcp->data.rr[0].ssrc) == 3027541352);
    CHECK(rtcp->data.rr[0].frac_lost() == 0);
    CHECK(rtcp->data.rr[0].cum_lost() == 0);
    CHECK(ntohl(rtcp->data.rr[0].jitter) == 413);

    // 2nd receiver report:
    CHECK(ntohl(rtcp->data.rr[1].ssrc) == 1926291140);
    CHECK(rtcp->data.rr[1].frac_lost() == 0);
    CHECK(rtcp->data.rr[1].cum_lost() == 0);
    CHECK(ntohl(rtcp->data.rr[1].jitter) == 391);
}

TEST_CASE("rtcp: parses receiver-estimated bandwidth reports", "[rtcp]") {

    const auto* rtcp = reinterpret_cast<const rtcp::hdr*>(test::rtcp_remb_buf);

    // rtcp common header:
    CHECK(rtcp->version() == 2);
    CHECK(rtcp->fb_fmt() == 15);
    CHECK(rtcp->padding() == 0);
    CHECK(rtcp->pt == 206);
    CHECK(rtcp->sender_ssrc == ntohl(3128394281));

    // remb message:
    CHECK(ntohl(rtcp->data.remb.source_ssrc) == 0);
    CHECK(rtcp->data.remb.remb == 0x424d4552); // "REMB"
    CHECK(rtcp->data.remb.num_ssrcs() == 1);
    CHECK(rtcp->data.remb.bit_rate() == 229389);
    // 1st remb ssrc:
    CHECK(rtcp->data.remb.ssrcs[0] == ntohl(1926291140));
}

TEST_CASE("rtcp: parses negative acknowledgements", "[rtcp]") {

    const auto* rtcp = reinterpret_cast<const rtcp::hdr*>(test::rtcp_nack_buf);

    // rtcp common header:
    CHECK(rtcp->version() == 2);
    CHECK(rtcp->fb_fmt() == 1);
    CHECK(rtcp->padding() == 0);
    CHECK(rtcp->pt == 205);
    CHECK(rtcp->sender_ssrc == ntohl(1));

    // nack-specific fields:
    CHECK(ntohl(rtcp->data.nack.ssrc) == 0x1c5ef618);
    CHECK(ntohs(rtcp->data.nack.pid) == 11564);
    CHECK(ntohs(rtcp->data.nack.blp) == 0);
}
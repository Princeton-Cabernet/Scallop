#include <catch.h>

#include "proto/rtp.h"
#include "stun_packets.h"
#include "rtp_rtcp_packets.h"
#include "data_plane_model.h"

using namespace p4sfu;
using namespace boost;

TEST_CASE("DataPlaneModel: sends specific packet tyoes to CPU port", "[data_plane_model]") {

    test::MockUDPServer udp;
    DataPlaneModel dp(&udp);

    std::vector<DataPlane::PktIn> pktsToCpu;

    dp.onPacketToController([&pktsToCpu](DataPlane& dp, DataPlane::PktIn pkt) {
        pktsToCpu.push_back(pkt);
    });

    SECTION("STUN binding request") {
        asio::ip::udp::endpoint ep{asio::ip::make_address_v4("0.0.0.0"), 2492};
        udp.receivePacket(ep, (char *) test::stun_bind_req, test::stun_bind_req_len);
        CHECK(pktsToCpu.size() == 1);
    }
}

TEST_CASE("DataPlaneModel: forwards packets along a forwarding association", "[data_plane_model]") {

    std::vector<test::MockUDPServer::Pkt> pktsSent;
    std::vector<DataPlane::PktIn> pktsToController;

    test::MockUDPServer udp;
    DataPlaneModel dp(&udp);

    dp.onPacketToController([&pktsToController](DataPlane& dp, DataPlane::PktIn pkt) {
        pktsToController.push_back(pkt);
    });

    udp.sentPacketHandler = [&pktsSent](const test::MockUDPServer::Pkt& pkt) {
        pktsSent.push_back(pkt);
    };

    dp.addStream(DataPlane::Stream{
        .src        = net::IPv4Port{net::IPv4{"1.1.1.1"}, 10001},
        .dst        = net::IPv4Port{net::IPv4{"2.2.2.2"}, 10002},
        .ssrc       = 0x6a70d0e8,
        .rtxSsrc    = 0x6a70d0e9,
        .egressPort = 1
    });

    asio::ip::udp::endpoint from{asio::ip::make_address_v4("1.1.1.1"), 10001};
    udp.receivePacket(from, (char*) test::rtp_buf1, sizeof(test::rtp_buf1));

    CHECK(dp.totalStatistics().rtpPkts == 1);
    CHECK(pktsToController.empty());
    CHECK(pktsSent.size() == 1);
    CHECK(pktsSent[0].to.address() == asio::ip::make_address_v4("2.2.2.2"));
    CHECK(pktsSent[0].to.port() == 10002);
}

/*
TEST_CASE("DataPlaneModel: copyRTPBuffer", "[sfu]") {

    auto from = test::full_rtp_av1;
    unsigned char to[1500];
    std::size_t fromLen = sizeof(test::full_rtp_av1), toLen = 1500;

    SECTION("copies a packet without modifications") {
        auto bytesWritten = DataPlaneModel::copyRTPBuffer(from, fromLen, to, toLen, {});
        CHECK(bytesWritten == 919);
        CHECK(std::memcmp(from, to, fromLen) == 0);
    }

    SECTION("copies a packet and rewrites the sequence number") {

        DataPlaneModel::RTPPktModifications mods { .rtpSeq = 23022 };
        auto bytesWritten = DataPlaneModel::copyRTPBuffer(from, fromLen, to, toLen, mods);
        CHECK(bytesWritten == 919);
        const auto* rtp = reinterpret_cast<const rtp::hdr*>(to);
        CHECK(ntohs(rtp->seq) == 23022);
        CHECK(std::memcmp(from, to, fromLen) > 0);
    }

    SECTION("copies a packet and adds or overrides RTP extensions") {

        auto rtpFrom = reinterpret_cast<const rtp::hdr*>(from);
        auto av1 = *(rtpFrom->extension(12));

        CHECK(av1.type == 12);
        CHECK(av1.len == 3);
        CHECK(av1.data[0] == 0x48);
        CHECK(av1.data[1] == 0x2d);
        CHECK(av1.data[2] == 0x93);

        // initialize using av1 dd from original packet:
        DataPlaneModel::RTPPktModifications mods { .av1 = av1 };

        mods.av1->len = 4;
        mods.av1->data[3] = 0b0100'0100;

        auto bytesWritten = DataPlaneModel::copyRTPBuffer(from, fromLen, to, toLen, mods);
        CHECK(bytesWritten == 919);
        const auto* rtpTo = reinterpret_cast<const rtp::hdr*>(to);
        CHECK(rtpTo->extension(12).has_value());
        auto av1To = *(rtpTo->extension(12));
        CHECK(av1To.type == 12);
        CHECK(av1To.len == 4);
        CHECK(av1To.data[0] == 0x48);
        CHECK(av1To.data[1] == 0x2d);
        CHECK(av1To.data[2] == 0x93);
        CHECK(av1To.data[3] == 0b0100'0100);
    }
}
*/
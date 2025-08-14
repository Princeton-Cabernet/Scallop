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

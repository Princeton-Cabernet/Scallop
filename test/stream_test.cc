#include <catch.h>

#include <stream.h>

#include "misc_data.h"
#include "sdp_lines.h"

using namespace p4sfu;

TEST_CASE("Stream: ()", "[stream]") {

    auto ssrcDescription = *(SDP::MediaDescription{test::sdpOfferMLines}.ssrcDescription);

    SECTION("constructs a send stream") {
        CHECK_NOTHROW([]() {
            Stream stream{SDP::MediaDescription{test::sdpOfferMLines}, test::sfuConfig};
            CHECK(stream.description().type == SDP::MediaDescription::Type::video);
            CHECK(stream.description().mid == "0");
            CHECK(stream.description().direction == SDP::Direction::sendonly);
            CHECK(stream.description().ssrcDescription.has_value());
            CHECK(stream.description().ssrcDescription->ssrcs.size() == 2);
            CHECK(stream.description().ssrcDescription->ssrcs[0].ssrc == 4071773911);
            CHECK(stream.description().ssrcDescription->ssrcs[1].ssrc == 2320959321);
            CHECK(stream.description().lines() == test::sdpOfferMLines);

            CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
            auto ss = std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig());

            CHECK(ss.iceUfrag == "zpyY");
            CHECK(ss.icePwd == "nKNWiTwEF8JF6Y2wAIG/hH0A");
            CHECK(ss.addr == net::IPv4Port{net::IPv4{"10.8.50.22"}, 60975});
            CHECK(ss.mainSSRC == 4071773911);
            CHECK(ss.rtxSSRC == 2320959321);
        }());
    }

    SECTION("constructs a receive stream") {

        CHECK_NOTHROW([&ssrcDescription]() {
            Stream stream{SDP::MediaDescription{test::sdpAnswerMLines}, test::sfuConfig,
                          ssrcDescription};

            CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()));
            auto rs = std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig());

            CHECK(rs.iceUfrag == "zpyY");
            CHECK(rs.icePwd == "nKNWiTwEF8JF6Y2wAIG/hH0A");
            CHECK(rs.addr == net::IPv4Port{net::IPv4{"10.8.50.22"}, 60975});
            CHECK(rs.mainSSRC == 4071773911);
            CHECK(rs.rtxSSRC == 2320959321);
        }());
    }

    SECTION("throws when a send stream is initialized with ssrc description") {
        CHECK_THROWS([&ssrcDescription]() {
            Stream stream{SDP::MediaDescription{test::sdpOfferMLines}, test::sfuConfig, ssrcDescription};
        }());
    }

    SECTION("throws when a receive stream is initialized without ssrc description") {
        CHECK_THROWS([]() {
            Stream stream{SDP::MediaDescription{test::sdpAnswerMLines}, test::sfuConfig};
        }());
    }
}

TEST_CASE("Stream: offer", "[stream]") {

    Stream stream{SDP::MediaDescription{test::sdpOfferMLines}, test::sfuConfig};
    CHECK(stream.offer(test::sfuConfig).lines() == test::sdpForwardOfferMLines);
}

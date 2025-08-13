
#include <catch.h>

#include <switch_agent_state.h>

using namespace p4sfu;


TEST_CASE("SwitchAgentState: addSendStream", "[switch_agent_state]") {

    SwitchAgentState s;
    CHECK_NOTHROW(
        s.addSendStream(1, 1, net::IPv4Port{"1.1.1.0", 49290}, MediaType::video, 1101, 1102));

    SECTION("adds a stream") {
        CHECK(s.sendStreams().size() == 2);
    }

    SECTION("throws when adding a send stream with a ssrc that is already present in the meeting") {
        CHECK(s.sendStreams().size() == 2);
        CHECK_THROWS(
            s.addSendStream(1, 1, net::IPv4Port{"1.1.1.0", 49290}, MediaType::video, 1101, 1102));
    }
}

TEST_CASE("SwitchAgentState: addReceiveStream", "[switch_agent_state]") {

    SwitchAgentState s;

    s.addSendStream(1, 1, net::IPv4Port{"1.1.1.0", 49290}, MediaType::video, 1101, 1102);
    CHECK(s.sendStreams().size() == 2);

    SECTION("throws when adding a receive stream that does not have a corresponding send stream") {

        CHECK_THROWS(s.addReceiveStream(1, 1, 2, net::IPv4Port{"1.2.1.0", 23292}, 3000, 3001));
        CHECK(s.receiveStreams().empty());
    }

    SECTION("adds a receive stream when there is a corresponding send stream") {

        CHECK_NOTHROW(s.addReceiveStream(1, 1, 2, net::IPv4Port{"1.2.1.0", 23292}, 1101, 1102));
        CHECK_NOTHROW(s.addReceiveStream(1, 1, 3, net::IPv4Port{"1.3.1.0", 12022}, 1101, 1102));
        CHECK(s.receiveStreams().size() == 4);
    }
}

TEST_CASE("SwitchAgentState: getSendStream", "[switch_agent_state]") {

    SwitchAgentState s;
    s.addSendStream(1, 1, net::IPv4Port{"1.1.1.0", 49290}, MediaType::video, 1101, 1102);

    SECTION("returns std::end if stream does not exist") {
        CHECK(s.getSendStream(1, 39403) == s.sendStreams().end());
    }

    SECTION("returns an iterator to the stream if it exists") {
        auto it = s.getSendStream(1, 1101);
        CHECK(it != s.sendStreams().end());
        CHECK(it->second.type == MediaType::video);
        CHECK(it->second.receiveStreamIds.empty());
    }
}

TEST_CASE("SwitchAgentState: getReceiveStream", "[switch_agent_state]") {

    SwitchAgentState s;
    s.addSendStream(1, 1, net::IPv4Port{"1.1.1.0", 49290}, MediaType::video, 1101, 1102);
    s.addReceiveStream(1, 1, 2, net::IPv4Port{"1.2.1.0", 23292}, 1101, 1102);
    s.addReceiveStream(1, 1, 3, net::IPv4Port{"1.3.1.0", 12022}, 1101, 1102);

    SECTION("by session id and participant id, returns std::end if stream does not exist") {
        CHECK(s.getReceiveStream(1, 39403, 3) == s.receiveStreams().end());
    }

    SECTION("by session id and participant id, returns an iterator to the stream if it exists") {
        auto it = s.getReceiveStream(1, 1101, 3);

        CHECK(it != s.receiveStreams().end());
        CHECK(it->second.type == MediaType::video);
        CHECK(it->second.sendStreamId == 0);
        CHECK(it->second.sendingParticipant == 1);
        CHECK(it->second.decodeTarget == av1::svc::L1T3::DecodeTarget::hi);
    }

    SECTION("by ip end point and ssrc, returns std::end if stream does not exist") {

        CHECK(s.getReceiveStream(net::IPv4Port{"1.2.9.0", 48222}, 6083493) == s.receiveStreams().end());
    }

    SECTION("by ip end point and ssrc, returns an iterator to the stream if it exists") {

        auto it = s.getReceiveStream(net::IPv4Port{"1.2.1.0", 23292}, 1101);
        CHECK(it != s.receiveStreams().end());
        CHECK(it->second.type == MediaType::video);
        CHECK(it->second.sendStreamId == 0);
        CHECK(it->second.sendingParticipant == 1);
        CHECK(it->second.decodeTarget == av1::svc::L1T3::DecodeTarget::hi);
    }
}

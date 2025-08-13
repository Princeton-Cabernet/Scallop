#include <catch.h>

#include <session.h>

#include "misc_data.h"
#include "sdp_lines.h"
#include "net/net.h"

using namespace p4sfu;

TEST_CASE("Session: ()", "[session]") {

    Session s1{0, test::sfuConfig};
    CHECK(s1.id() == 0);

    Session s2{1,  test::sfuConfig};
    CHECK(s2.id() == 1);
}

TEST_CASE("Session: participantExists(ip, port)", "[session]") {

    Session s{0, test::sfuConfig};
    unsigned _;
    _ = s.addParticipant("127.0.0.1", 49222);
    _ = s.addParticipant("127.0.0.1", 49223);

    CHECK(s.participantExists("127.0.0.1", 49222));
    CHECK(s.participantExists("127.0.0.1", 49223));
    CHECK_FALSE(s.participantExists("23.0.2.1", 2299));
    CHECK_FALSE(s.participantExists("127.0.0.1", 49224));
}

TEST_CASE("Session: participantExists(id)", "[session]") {

    Session s{0,test::sfuConfig};
    auto id1 = s.addParticipant("127.0.0.1", 49222);
    auto id2 = s.addParticipant("127.0.0.1", 49223);
    CHECK(s.participantExists(id1));
    CHECK(s.participantExists(id2));
    CHECK_FALSE(s.participantExists(2392));
    CHECK_FALSE(s.participantExists(238929));
}

TEST_CASE("Session: addParticipant", "[session]") {

    Session s1{0, test::sfuConfig};
    CHECK(s1.addParticipant("127.0.0.1", 49222) == 1);
    CHECK(s1.addParticipant("127.0.0.1", 49223) == 2);

    SECTION("throws when participant exists") {
        Session s2{0, test::sfuConfig};
        CHECK(s2.addParticipant("127.0.0.1", 49222) == 1);
        CHECK_THROWS(s2.addParticipant("127.0.0.1", 49222));
    }
}

TEST_CASE("Session: countParticipants", "[session]") {

    Session s{0, test::sfuConfig};
    unsigned _;
    CHECK(s.countParticipants() == 0);
    _ = s.addParticipant("127.0.0.1", 49222);
    CHECK(s.countParticipants() == 1);
    _ = s.addParticipant("127.0.0.1", 49223);
    CHECK(s.countParticipants() == 2);
}

TEST_CASE("Session: getParticipant", "[session]") {

    Session s{0, test::sfuConfig};
    auto id = s.addParticipant("127.0.0.1", 49222);
    auto& p = s.getParticipant(id);

    CHECK(p.id() == id);

    SECTION("throws when participant does not exist") {
        CHECK_THROWS(s.getParticipant(293));
    }
}

TEST_CASE("Session: operator[]", "[session]") {

    Session s{0, test::sfuConfig};
    auto id = s.addParticipant("127.0.0.1", 49222);

    auto& p = s[id];
    CHECK(p.id() == id);

    SECTION("throws when participant does not exist") {
        CHECK_THROWS(s[293]);
    }
}

TEST_CASE("Session: participants", "[session]") {

    Session s{0, test::sfuConfig};
    auto id = s.addParticipant("127.0.0.1", 49222);
    auto &p = s.getParticipant(id);

    const auto& participants = s.participants();
    CHECK(participants.size() == 1);
    CHECK(participants.begin()->first == id);
}

TEST_CASE("Session: removeParticipant", "[session]") {

    Session s{0, test::sfuConfig};
    auto id = s.addParticipant("127.0.0.1", 49222);

    CHECK(s.countParticipants() == 1);
    CHECK_NOTHROW(s.removeParticipant(id));
    CHECK(s.countParticipants() == 0);

    SECTION("throws when participant does not exist") {
        CHECK_THROWS(s.removeParticipant(293));
    }
}

TEST_CASE("Session: addOffer", "[session]") {

    Session s{0, test::sfuConfig};
    s.onNewStream([](const Session&, unsigned participantId, const Stream&) {});

    CHECK(s.id() == 0);
    CHECK(s.countParticipants() == 0);
    CHECK_FALSE(s.signalingActive());

    SECTION("throws when participant does not exist") {

        SDP::SessionDescription offer{test::sdpOfferLines1};
        CHECK_THROWS_AS(s.addOffer(4, offer), std::logic_error);
        CHECK_FALSE(s.signalingActive());
    }

    SECTION("adds a stream to a participant") {

        auto partId = s.addParticipant("127.0.0.1", 49222);

        SDP::SessionDescription offer{test::sdpOfferLines1};

        s.addOffer(partId, offer);
        CHECK(s.getParticipant(partId).outgoingStreams().size() == 1);
        CHECK(s.signalingActive());
    }

    SECTION("adds the same stream only once") {

        auto partId = s.addParticipant("127.0.0.1", 49222);

        SDP::SessionDescription offer{test::sdpOfferLines1};

        CHECK_NOTHROW(s.addOffer(partId, offer));
        CHECK_THROWS_AS(s.addOffer(partId, offer), std::logic_error);
        CHECK(s.getParticipant(partId).outgoingStreams().size() == 1);
        CHECK(s.signalingActive());
    }
}

TEST_CASE("Session: getAnswer", "[session]") {

    Session s{0, test::sfuConfig};
    s.onNewStream([](const Session&, unsigned participantId, const Stream&) {});

    SECTION("throws when participant does not exist") {

        CHECK_THROWS_AS(s.getAnswer(0), std::logic_error);
    }

    SECTION("throws when there is no pending answer") {

        auto partId = s.addParticipant("127.0.0.1", 49222);
        CHECK_THROWS_AS(s.getAnswer(partId), std::logic_error);
    }

    SECTION("returns an answer for the pending offer") {

        auto participantId = s.addParticipant("127.0.0.1", 49222);
        CHECK_THROWS_AS(s.getAnswer(participantId), std::logic_error);

        SDP::SessionDescription offer{test::sdpOfferLines1};
        s.addOffer(participantId, offer);

        auto answer = s.getAnswer(participantId);
        CHECK(answer.lines() == test::sdpAnswerLines1);
    }
}

TEST_CASE("Session: getOffer", "[session]") {

    Session s{0, test::sfuConfig};
    s.onNewStream([](const Session&, unsigned participantId, const Stream&) {});

    SECTION("throws when participant does not exist") {

        CHECK_THROWS_AS(s.getOffer(0), std::logic_error);
    }

    SECTION("generates an offer including all active media streams") {

        auto part1 = s.addParticipant("127.0.0.1", 49200);
        auto part2 = s.addParticipant("127.0.0.1", 49201);

        SDP::SessionDescription offerFromPart1{test::sdpOfferLines1};
        s.addOffer(part1, offerFromPart1);

        auto offerForPart2 = s.getOffer(part2);
        CHECK(offerForPart2.lines() == test::sdpForwardOfferLines1);
    }
}

TEST_CASE("Session: addAnswer", "[session]") {

    Session s{0, test::sfuConfig};
    s.onNewStream([](const Session&, unsigned participantId, const Stream&) {});

    SDP::SessionDescription offer(test::sdpOfferLines1);

    SDP::SessionDescription answer{test::sdpForwardAnswerLines};

    auto part1 = s.addParticipant("127.0.0.1", 49200);
    auto part2 = s.addParticipant("127.0.0.1", 49201);

    s.addOffer(part1, offer);


    SECTION("throws when no answer pending does not exist") {

        CHECK_THROWS_AS(s.addAnswer(part1, answer), std::logic_error);
    }

    SECTION("adds answer") {

        //TODO: this is currently broken: need to implement the ability to get offers for a joining
        //      participant

        /*
        auto offer1 = s.getOffer(part1);
        auto offer2 = s.getOffer(part2);

        CHECK(s.signalingActive());

        s.addAnswer(part1, answer);
        s.addAnswer(part2, answer);

        CHECK_FALSE(s.signalingActive());
        */
    }
}


TEST_CASE("Session: integration: test 02/09/24", "[session]") {

    // (1) Participant 1 joins
    // (2) Participant 2 joins
    // (3) Participant 1 starts video

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session& session, unsigned participantId,
            const Stream& stream) {
        switch (stream.description().direction.value()) {
            case SDP::Direction::sendonly:
                CHECK(std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                sendStreams.push_back(stream);
                break;
            case SDP::Direction::recvonly:
                CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()));
                CHECK(std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()).mainSSRC);
                receiveStreams.push_back(stream);
                break;
            default:
                FAIL("stream is neither sendonly nor recvonly");
        }
    });

    SDP::SessionDescription offerFromP1{test::msg1}, offerForP2, answerFromP2{test::msg4}, answerForP1;

    auto p1 = s.addParticipant("127.0.0.1", 1);
    auto p2 = s.addParticipant("127.0.0.1", 2);

    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.empty());
    CHECK(receiveStreams.empty());

    CHECK_NOTHROW(s.addOffer(p1, offerFromP1));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.empty());

    CHECK_NOTHROW(offerForP2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.empty());

    CHECK_NOTHROW(s.addAnswer(p2, answerFromP2));
    CHECK(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 1);

    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[p1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[p2].signalingState() == Participant::SignalingState::stable);
    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 1);

    auto sendStream = sendStreams[0].dataPlaneConfig<Stream::SendStreamConfig>();
    auto receiveStream = receiveStreams[0].dataPlaneConfig<Stream::ReceiveStreamConfig>();

    CHECK(sendStream.mainSSRC == 4071773911);
    CHECK(sendStream.rtxSSRC == 2320959321);

    CHECK(receiveStream.mainSSRC == 4071773911);
    CHECK(receiveStream.rtxSSRC == 2320959321);
    CHECK(receiveStream.rtcpSSRC == 227949905);
}

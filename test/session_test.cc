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
/*
TEST_CASE("Session: integration: test 01/23/24", "[session]") {

    // Part 1:
    // (1) Participant 1 joins
    // (2) Participant 1 starts video
    // (3) Participant 2 joins

    Session s{0, test::sfuConfig};

    std::vector<Stream> sendStreams, receiveStreams;

    s.onNewStream([&sendStreams, &receiveStreams](const Session& session, const Stream& stream) {
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

    auto p1 = s.addParticipant("127.0.0.1", 1);

    CHECK_FALSE(s.signalingActive());

    // mid=0, ip=10.8.50.22, port=60975, type=video, ssrc=4071773911, 2320959321
    SDP::SessionDescription offer{test::msg1};
    s.addOffer(p1, offer);

    CHECK(s.signalingActive());

    SDP::SessionDescription answerForP1;
    CHECK_NOTHROW(answerForP1 = s.getAnswer(p1));
    CHECK(answerForP1.mediaDescriptions.size() == 1);
    CHECK(answerForP1.mediaDescriptions[0].mid == "0");
    CHECK(answerForP1.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(answerForP1.mediaDescriptions[0].direction == SDP::Direction::recvonly);

    CHECK(answerForP1.mediaDescriptions[0].mediaFormats.size() == 2);
    CHECK(answerForP1.mediaDescriptions[0].mediaFormats[0].pt == 45);
    CHECK(answerForP1.mediaDescriptions[0].mediaFormats[1].pt == 46);
    CHECK_THAT(answerForP1.mediaDescriptions[0].mediaFormats[0].fbTypes,
               Catch::Matchers::UnorderedEquals(test::sfuConfig.videoFbTypes()));
    CHECK_FALSE(answerForP1.mediaDescriptions[0].msid.has_value());
    CHECK_FALSE(answerForP1.mediaDescriptions[0].ssrcDescription.has_value());

    // cannot get a second answer:
    CHECK_THROWS(answerForP1 = s.getAnswer(p1));

    CHECK_FALSE(s.signalingActive());

    auto p2 = s.addParticipant("127.0.0.1", 2);

    CHECK(s.countStreams() == 1);

    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.empty());

    {
        auto stream = sendStreams[0];
        CHECK(stream.description().type == SDP::MediaDescription::Type::video);
        CHECK(stream.description().direction == SDP::Direction::sendonly);
        CHECK(stream.description().ssrcDescription);
        CHECK(stream.description().ssrcDescription->ssrcs.size() == 2);
        CHECK(stream.description().ssrcDescription->ssrcs[0].ssrc == 4071773911);
        CHECK(stream.description().ssrcDescription->ssrcs[1].ssrc == 2320959321);
    }

    SDP::SessionDescription offerForP2;
    CHECK_NOTHROW(offerForP2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(offerForP2.mediaDescriptions.size() == 1);
    CHECK(offerForP2.mediaDescriptions[0].mid == "0");
    CHECK(offerForP2.mediaDescriptions[0].type == SDP::MediaDescription::Type::video);
    CHECK(offerForP2.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offerForP2.mediaDescriptions[0].ssrcDescription);
    CHECK(offerForP2.mediaDescriptions[0].ssrcDescription->ssrcs.size() == 2);
    CHECK(offerForP2.mediaDescriptions[0].ssrcDescription->ssrcs[0].ssrc == 4071773911);
    CHECK(offerForP2.mediaDescriptions[0].ssrcDescription->ssrcs[1].ssrc == 2320959321);
    CHECK(offerForP2.mediaDescriptions[0].msid->appData == "42f41fd3-296c-468a-8bd6-b8c4bf60daa9");
    CHECK_THAT(offerForP2.mediaDescriptions[0].mediaFormats[0].fbTypes,
               Catch::Matchers::UnorderedEquals(test::sfuConfig.videoFbTypes()));

    SDP::SessionDescription answerFromP2{test::msg4};
    s.addAnswer(p2, answerFromP2);

    CHECK(sendStreams.size() == 1);
    CHECK(receiveStreams.size() == 1);
    CHECK(s.countStreams() == 1);
    CHECK_FALSE(s.signalingActive());

    {
        auto stream = receiveStreams[0];
        CHECK(stream.description().type == SDP::MediaDescription::Type::video);
        CHECK(stream.description().direction == SDP::Direction::recvonly);

        CHECK(stream.description().ssrcDescription.has_value());
        CHECK(stream.description().ssrcDescription->ssrcs.size() == 2);
        CHECK(stream.description().ssrcDescription->ssrcs[0].ssrc == 227949905);
        CHECK(stream.description().ssrcDescription->ssrcs[1].ssrc == 2545253765);

        CHECK(std::holds_alternative<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig()));

        auto dpConfig = stream.dataPlaneConfig<Stream::ReceiveStreamConfig>();
        CHECK(dpConfig.mainSSRC == 4071773911);
        CHECK(dpConfig.rtxSSRC == 2320959321);
        CHECK(dpConfig.rtcpSSRC == 227949905);
    }

    // Part 2:
    // (4) Participant 2 starts video

    SDP::SessionDescription offer2{test::msg5};
    s.addOffer(p2, offer2);
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s.countStreams() == 2);
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 1);

    SDP::SessionDescription offerForP1;
    CHECK_NOTHROW(offerForP1 = s.getOffer(p1));

    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[2].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(offerForP1.mediaDescriptions.size() == 2);

    CHECK(offerForP1.mediaDescriptions[0].mid == "0");
    CHECK(offerForP1.mediaDescriptions[0].direction == SDP::Direction::recvonly);
    CHECK(offerForP1.mediaDescriptions[0].msid->appData == "42f41fd3-296c-468a-8bd6-b8c4bf60daa9");
    CHECK_FALSE(offerForP1.mediaDescriptions[0].ssrcDescription.has_value());
    CHECK(offerForP1.mediaDescriptions[1].mid == "1");
    CHECK(offerForP1.mediaDescriptions[1].direction == SDP::Direction::sendonly);
    CHECK(offerForP1.mediaDescriptions[1].msid->appData == "42f41fd3-296c-468a-8bd6-b8c4bf60daaa");
    CHECK(offerForP1.mediaDescriptions[1].ssrcDescription.has_value());
    CHECK(offerForP1.mediaDescriptions[1].ssrcDescription->ssrcs[0].ssrc == 4071773912);
    CHECK(offerForP1.mediaDescriptions[1].ssrcDescription->ssrcs[1].ssrc == 2320959322);

    // add answer from p1
    SDP::SessionDescription answerFromP1{test::msg6};
    s.addAnswer(p1, answerFromP1);
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(s.countStreams() == 2);
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 2);

    // get answer for p2
    SDP::SessionDescription answerForP2;
    CHECK_NOTHROW(answerForP2 = s.getAnswer(p2));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);

    CHECK(answerForP2.mediaDescriptions.size() == 2);
    CHECK(answerForP2.mediaDescriptions[0].mid == "0");
    CHECK(answerForP2.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(answerForP2.mediaDescriptions[0].msid->appData == "42f41fd3-296c-468a-8bd6-b8c4bf60daa9");
    CHECK(answerForP2.mediaDescriptions[0].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(answerForP2.mediaDescriptions[0].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(answerForP2.mediaDescriptions[0].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(answerForP2.mediaDescriptions[0].ssrcDescription.has_value());
    CHECK(answerForP2.mediaDescriptions[1].mid == "1");
    CHECK(answerForP2.mediaDescriptions[1].direction == SDP::Direction::recvonly);
    CHECK_FALSE(answerForP2.mediaDescriptions[1].msid.has_value());
    CHECK(answerForP2.mediaDescriptions[1].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(answerForP2.mediaDescriptions[1].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(answerForP2.mediaDescriptions[1].iceCandidates[0].port == test::sfuConfig.port());
    CHECK_FALSE(answerForP2.mediaDescriptions[1].ssrcDescription.has_value());

    CHECK(s.countStreams() == 2);
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 2);

    // Part 3:
    // (5) Participant 3 joins

    auto p3 = s.addParticipant("127.0.0.1", 3);

    // get offer for p3
    SDP::SessionDescription offerForP3;
    CHECK_NOTHROW(offerForP3 = s.getOffer(p3));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::awaitAnswer);

    CHECK(s.countStreams() == 2);
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 2);

    CHECK(offerForP3.mediaDescriptions.size() == 2);
    CHECK(offerForP3.mediaDescriptions[0].mid == "0");
    CHECK(offerForP3.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offerForP3.mediaDescriptions[0].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP3.mediaDescriptions[0].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP3.mediaDescriptions[0].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP3.mediaDescriptions[0].ssrcDescription.has_value());
    CHECK(offerForP3.mediaDescriptions[1].mid == "1");
    CHECK(offerForP3.mediaDescriptions[1].direction == SDP::Direction::sendonly);
    CHECK(offerForP3.mediaDescriptions[1].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP3.mediaDescriptions[1].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP3.mediaDescriptions[1].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP3.mediaDescriptions[1].ssrcDescription.has_value());

    // add answer from p3
    CHECK_NOTHROW(s.addAnswer(p3, SDP::SessionDescription{test::msg7}));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::stable);

    CHECK(s.countStreams() == 2);
    CHECK(sendStreams.size() == 2);
    CHECK(receiveStreams.size() == 4);

    // Part 4:
    // (6) Participant 3 starts video

    // add offer from p3
    SDP::SessionDescription offer3{test::msg8};
    CHECK_NOTHROW(s.addOffer(p3, offer3));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::gotOffer);
    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 4);


    // get offer for p1
    SDP::SessionDescription offerForP1_2;
    CHECK_NOTHROW(offerForP1_2 = s.getOffer(p1));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(offerForP1_2.mediaDescriptions.size() == 3);
    CHECK(offerForP1_2.mediaDescriptions[0].mid == "0");
    CHECK(offerForP1_2.mediaDescriptions[0].direction == SDP::Direction::recvonly);
    CHECK(offerForP1_2.mediaDescriptions[0].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[0].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[0].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP1_2.mediaDescriptions[1].mid == "1");
    CHECK(offerForP1_2.mediaDescriptions[1].direction == SDP::Direction::sendonly);
    CHECK(offerForP1_2.mediaDescriptions[1].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[1].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[1].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP1_2.mediaDescriptions[2].mid == "2");
    CHECK(offerForP1_2.mediaDescriptions[2].direction == SDP::Direction::sendonly);
    CHECK(offerForP1_2.mediaDescriptions[2].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[2].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP1_2.mediaDescriptions[2].iceCandidates[0].port == test::sfuConfig.port());

    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 4);


    // get offer for p2
    SDP::SessionDescription offerForP2_2;
    CHECK_NOTHROW(offerForP2_2 = s.getOffer(p2));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[3].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(offerForP2_2.mediaDescriptions.size() == 3);
    CHECK(offerForP2_2.mediaDescriptions[0].mid == "0");
    CHECK(offerForP2_2.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(offerForP2_2.mediaDescriptions[0].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[0].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[0].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP2_2.mediaDescriptions[1].mid == "1");
    CHECK(offerForP2_2.mediaDescriptions[1].direction == SDP::Direction::recvonly);
    CHECK(offerForP2_2.mediaDescriptions[1].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[1].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[1].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(offerForP2_2.mediaDescriptions[2].mid == "2");
    CHECK(offerForP2_2.mediaDescriptions[2].direction == SDP::Direction::sendonly);
    CHECK(offerForP2_2.mediaDescriptions[2].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[2].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(offerForP2_2.mediaDescriptions[2].iceCandidates[0].port == test::sfuConfig.port());

    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 4);

    // add answer from p1
    SDP::SessionDescription answerFromP1_2{test::msg9};
    CHECK_NOTHROW(s.addAnswer(p1, answerFromP1_2));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::awaitAnswer);
    CHECK(s[3].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 5);

    // add answer from p2
    SDP::SessionDescription answerFromP2_2{test::msg10};
    CHECK_NOTHROW(s.addAnswer(p2, answerFromP2_2));
    CHECK(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::gotOffer);

    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 6);

    // get answer for p3
    SDP::SessionDescription answerForP3;
    CHECK_NOTHROW(answerForP3 = s.getAnswer(p3));
    CHECK_FALSE(s.signalingActive());
    CHECK(s[1].signalingState() == Participant::SignalingState::stable);
    CHECK(s[2].signalingState() == Participant::SignalingState::stable);
    CHECK(s[3].signalingState() == Participant::SignalingState::stable);

    CHECK(s.countStreams() == 3);
    CHECK(sendStreams.size() == 3);
    CHECK(receiveStreams.size() == 6);

    CHECK(answerForP3.mediaDescriptions.size() == 3);
    CHECK(answerForP3.mediaDescriptions[0].mid == "0");
    CHECK(answerForP3.mediaDescriptions[0].direction == SDP::Direction::sendonly);
    CHECK(answerForP3.mediaDescriptions[0].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[0].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[0].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(answerForP3.mediaDescriptions[1].mid == "1");
    CHECK(answerForP3.mediaDescriptions[1].direction == SDP::Direction::sendonly);
    CHECK(answerForP3.mediaDescriptions[1].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[1].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[1].iceCandidates[0].port == test::sfuConfig.port());
    CHECK(answerForP3.mediaDescriptions[2].mid == "2");
    CHECK(answerForP3.mediaDescriptions[2].direction == SDP::Direction::recvonly);
    CHECK(answerForP3.mediaDescriptions[2].connectionData->connectionAddress == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[2].iceCandidates[0].address == test::sfuConfig.ip());
    CHECK(answerForP3.mediaDescriptions[2].iceCandidates[0].port == test::sfuConfig.port());
}
*/
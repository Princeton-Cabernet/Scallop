#include <catch.h>
#include <rpc.h>

#include "rpc_messages.h"

TEST_CASE("rpc: sw::Hello", "[rpc]") {

    SECTION("serialize w/o switchId") {
        rpc::sw::Hello hello;
        CHECK(hello.to_json() == test::hello_rpc_json);
    }

    SECTION("deserialize w/o switchId") {
        rpc::sw::Hello hello{json::json::parse(test::hello_rpc_json)};
        CHECK(hello.type == "hello");
    }

    SECTION("serialize w/ switchId") {
        rpc::sw::Hello hello;
        hello.switchId = 1;
        CHECK(hello.to_json() == test::sw_hello_switch_rpc_json);
    }

    SECTION("deserialize w/ switchId") {
        rpc::sw::Hello hello{json::json::parse(test::sw_hello_switch_rpc_json)};
        CHECK(hello.type == "hello");
        CHECK(hello.switchId.has_value());
        CHECK(hello.switchId == 1);
    }
}

TEST_CASE("rpc: sw::AddParticipant", "[rpc]") {

    SECTION("serialize") {
        rpc::sw::AddParticipant addPart;
        addPart.sessionId = 1;
        addPart.participantId = 2;
        addPart.ip = net::IPv4{"1.2.3.4"};
        CHECK(addPart.to_json() == test::sw_add_participant_rpc_json);
    }

    SECTION("deserialize") {
        rpc::sw::AddParticipant addPart{json::json::parse(test::sw_add_participant_rpc_json)};
        CHECK(addPart.type == "add_participant");
        CHECK(addPart.sessionId == 1);
        CHECK(addPart.participantId == 2);
        CHECK(addPart.ip == net::IPv4{"1.2.3.4"});
    }
}

TEST_CASE("rpc: sw::AddStream", "[rpc]") {

    SECTION("serialize") {
        rpc::sw::AddStream addStream;
        addStream.direction = SDP::Direction::sendonly;
        addStream.sessionId = 1;
        addStream.participantId = 2;
        addStream.ip = net::IPv4{"1.2.3.4"};
        addStream.port = 2322;
        addStream.iceUfrag = "fksl3";
        addStream.icePwd = "dsfjsdksddfkdmfdk03";
        addStream.mainSSRC = 24900303;
        addStream.rtxSSRC = 329439892;
        addStream.rtcpSSRC = 52902399;
        addStream.rtcpRtxSSRC = 24829929;
        addStream.mediaType = p4sfu::MediaType::video;
        CHECK(addStream.to_json() == test::sw_add_stream_rpc_json);
    }

    SECTION("deserialize") {
        rpc::sw::AddStream addStream{json::json::parse(test::sw_add_stream_rpc_json)};
        CHECK(addStream.type == "add_stream");
        CHECK(addStream.sessionId == 1);
        CHECK(addStream.participantId == 2);
        CHECK(addStream.ip == net::IPv4{"1.2.3.4"});
        CHECK(addStream.port == 2322);
        CHECK(addStream.iceUfrag == "fksl3");
        CHECK(addStream.icePwd == "dsfjsdksddfkdmfdk03");
        CHECK(addStream.mainSSRC == 24900303);
        CHECK(addStream.rtxSSRC == 329439892);
        CHECK(addStream.rtcpSSRC == 52902399);
        CHECK(addStream.rtcpRtxSSRC == 24829929);
        CHECK(addStream.mediaType == p4sfu::MediaType::video);
    }
}

TEST_CASE("rpc: cl::Hello", "[rpc]") {

    SECTION("serialize w/o sessionId") {
        rpc::cl::Hello hello;
        CHECK(hello.to_json() == test::hello_rpc_json);
    }

    SECTION("deserialize w/o sessionId") {
        rpc::cl::Hello hello{json::json::parse(test::hello_rpc_json)};
        CHECK(hello.type == "hello");
    }

    SECTION("serialize w/ sessionId") {
        rpc::cl::Hello hello;
        hello.sessionId = 1234;
        CHECK(hello.to_json() == test::cl_hello_session_rpc_json);
    }

    SECTION("deserialize w/ sessionId") {
        rpc::cl::Hello hello{json::json::parse(test::cl_hello_session_rpc_json)};
        CHECK(hello.type == "hello");
        CHECK(hello.sessionId.has_value());
        CHECK(hello.sessionId == 1234);
    }

    SECTION("serialize w/ participantId") {
        rpc::cl::Hello hello;
        hello.participantId = 429;
        CHECK(hello.to_json() == test::cl_hello_participant_rpc_json);
    }

    SECTION("deserialize w/participantId") {
        rpc::cl::Hello hello{json::json::parse(test::cl_hello_participant_rpc_json)};
        CHECK(hello.type == "hello");
        CHECK(hello.participantId.has_value());
        CHECK(hello.participantId == 429);
    }
}

TEST_CASE("rpc: cl::Offer", "[rpc]") {

    SECTION("serialize") {
        rpc::cl::Offer offer;
        offer.sdp = "TEST_SDP";
        offer.participantId = 5;
        CHECK(offer.to_json() == test::cl_offer_rpc_json);
    }

    SECTION("deserialize") {
        rpc::cl::Offer offer{json::json::parse(test::cl_offer_rpc_json)};
        CHECK(offer.type == "offer");
        CHECK(offer.sdp == "TEST_SDP");
        CHECK(offer.participantId == 5);
    }
}

TEST_CASE("rpc: cl::Answer", "[rpc]") {

    SECTION("serialize") {
        rpc::cl::Answer answer;
        answer.sdp = "TEST_SDP";
        answer.participantId = 5;
        CHECK(answer.to_json() == test::cl_answer_rpc_json);
    }

    SECTION("deserialize") {
        rpc::cl::Answer answer{json::json::parse(test::cl_answer_rpc_json)};
        CHECK(answer.type == "answer");
        CHECK(answer.sdp == "TEST_SDP");
        CHECK(answer.participantId == 5);
    }
}

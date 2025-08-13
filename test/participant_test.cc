#include <catch.h>

#include <session.h>
#include <participant.h>
#include <stream.h>

#include "misc_data.h"
#include "sdp_lines.h"

using namespace p4sfu;

TEST_CASE("Participant: addOutgoingStream", "[participant]") {

    Session s{0, test::sfuConfig};
    auto id = s.addParticipant("127.0.0.1", 23292);
    auto participant = s.getParticipant(id);

    SDP::SessionDescription sdp(test::sdpOfferLines1);
    auto media = sdp.mediaDescriptions[0];

    participant.addOutgoingStream(Stream{media, test::sfuConfig});

    CHECK(participant.outgoingStreams().size() == 1);
    const auto& stream = participant.outgoingStreams()[0];

    CHECK(stream.description().msid.has_value());
    CHECK(stream.description().msid->appData == "42f41fd3-296c-468a-8bd6-b8c4bf60daa9");
}

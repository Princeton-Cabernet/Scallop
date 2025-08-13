
#include "participant.h"

#include <algorithm>

unsigned p4sfu::Participant::id() const {
    
    return _id;
}

void p4sfu::Participant::addOutgoingStream(const Stream& s) {

    _sender.streams.push_back(s);
}

void p4sfu::Participant::addIncomingStream(unsigned sendingParticipant, Stream& s) {

    _receivers[sendingParticipant].streams.push_back(s);
}

/*
void p4sfu::Participant::addOutgoingStream(const Stream& s) {

    _outgoingStreams.push_back(s);
}

void  p4sfu::Participant::addIncomingStream(const Stream& s) {

    _incomingStreams.push_back(s);
}
*/
const std::vector<p4sfu::Stream>& p4sfu::Participant::outgoingStreams() const {

    return _sender.streams;
}

const std::vector<p4sfu::Stream> p4sfu::Participant::incomingStreams() const {

    std::vector<Stream> v;

    for (const auto& [_, r] : _receivers) {
        for (const auto& s: r.streams) {
            v.push_back(s);
        }
    }

    return v;
}

[[nodiscard]] bool p4sfu::Participant::sendsStreamWithMid(const std::string& mid) const {

    return _sender.hasStreamWithMid(mid);
}

[[nodiscard]] const p4sfu::Stream& p4sfu::Participant::sendStreamWithMid(const std::string& mid) const {

    if (!_sender.hasStreamWithMid(mid)) {
        throw std::logic_error("Participant: sendStreamWithMid: participant does'nt send this mid");
    } else {
        return *(std::find_if(_sender.streams.begin(), _sender.streams.end(),
            [&mid](const Stream& s) {
                return s.description().mid.has_value() && *s.description().mid == mid;
        }));
    }
}


/*
bool p4sfu::Participant::Sender::sendsStreamWithMid(const std::string& mid) const {

    return std::find_if(_outgoingStreams.begin(), _outgoingStreams.end(),
        [&mid](const p4sfu::Stream& s) {
            return s.description().mid.value() == mid;
        }) != _outgoingStreams.end();
}

bool p4sfu::Participant::Receiver::receivesStreamWithMid(const std::string &mid) const {

    return std::find_if(_incomingStreams.begin(), _incomingStreams.end(),
        [&mid](const p4sfu::Stream &s) {
            return s.description().mid.value() == mid;
        }) != _incomingStreams.end();
}
*/

void p4sfu::Participant::Transceiver::addStream(const Stream& stream) {

    streams.push_back(stream);
}

void p4sfu::Participant::Transceiver::updateICECandidatesFromSessionDescription(
    const SDP::SessionDescription& sdp) {

    // go through all ice candidates in offer and add to participant if not yet added
    for (const auto& media: sdp.mediaDescriptions) {
        for (const auto &iceCandidate: media.iceCandidates) {
            if (!hasICECandidate(iceCandidate)) {
                addICECandidate(iceCandidate);
            }
        }
    }
}

void p4sfu::Participant::Transceiver::addICECandidate(const SDP::ICECandidate& c) {

    _iceCandidates.push_back(c);
}

const std::vector<SDP::ICECandidate>& p4sfu::Participant::Transceiver::iceCandidates() const {

    return _iceCandidates;
}

bool p4sfu::Participant::Transceiver::hasICECandidate(const SDP::ICECandidate& c) const {

    return std::find(_iceCandidates.begin(), _iceCandidates.end(), c) != _iceCandidates.end();
}

const SDP::ICECandidate& p4sfu::Participant::Transceiver::highestPriorityICECandidate() const {

    return *std::max_element(_iceCandidates.begin(), _iceCandidates.end(),
        [](const auto& a, const auto& b) { return a.priority < b.priority; });
}

[[nodiscard]] bool p4sfu::Participant::Transceiver::hasStreamWithMid(const std::string& mid) const {

    return std::find_if(streams.begin(), streams.end(), [&mid](const Stream& s) {
        return s.description().mid.has_value() && *s.description().mid == mid;
    }) != streams.end();
}

[[nodiscard]] p4sfu::Participant::SignalingState p4sfu::Participant::signalingState() const {
    return _signalingState;
}
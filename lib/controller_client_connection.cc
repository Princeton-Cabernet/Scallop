
#include "controller_client_connection.h"
#include <nlohmann/json.hpp>
#include "rpc.h"

namespace json = nlohmann;

bool p4sfu::ClientMeta::operator==(const p4sfu::ClientMeta& other) const {
    return std::make_tuple(sessionId, participantId)
           == std::make_tuple(other.sessionId, other.participantId);
}

bool p4sfu::ClientMeta::operator<(const p4sfu::ClientMeta& other) const {
    return std::make_tuple(sessionId, participantId)
           < std::make_tuple(other.sessionId, other.participantId);
}

p4sfu::ControllerClientConnection::ControllerClientConnection(SecureWebSocketSession<ClientMeta>& s)
    : _ws(s.shared_from_this()) { }

void p4sfu::ControllerClientConnection::sendHello() {

    rpc::cl::Hello m;

    if (_ws->meta().sessionId) {
        m.sessionId = _ws->meta().sessionId;
    }

    if (_ws->meta().participantId) {
        m.participantId = _ws->meta().participantId;
    }

    _ws->write(m.to_json());
}

void p4sfu::ControllerClientConnection::sendOffer(const SDP::SessionDescription& offer,
                                                  unsigned relatedParticipantId) {

    rpc::cl::Offer msg;
    msg.sdp = offer.escapedString();
    msg.participantId = relatedParticipantId;
    _ws->write(msg.to_json());
}

void p4sfu::ControllerClientConnection::sendAnswer(const SDP::SessionDescription& answer,
                                                   unsigned relatedParticipantId) {

    rpc::cl::Answer msg;
    msg.sdp = answer.escapedString();
    msg.participantId = relatedParticipantId;
    _ws->write(msg.to_json());
}

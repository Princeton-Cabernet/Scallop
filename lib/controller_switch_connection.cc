
//#include "rpc.h"
#include <nlohmann/json.hpp>
#include "controller_switch_connection.h"
#include "rpc.h"

namespace json = nlohmann;

bool p4sfu::SwitchMeta::operator==(const p4sfu::SwitchMeta& other) const {

    return id == other.id;
}

bool p4sfu::SwitchMeta::operator<(const p4sfu::SwitchMeta& other) const {

    return id < other.id;
}

p4sfu::ControllerSwitchConnection::ControllerSwitchConnection(WebSocketSession<SwitchMeta>& s)
    : _ws(s.shared_from_this()) { }

void p4sfu::ControllerSwitchConnection::sendHello() {

    _ws->write(rpc::sw::Hello{}.to_json());
}

void p4sfu::ControllerSwitchConnection::addParticipant(
    unsigned sessionId, unsigned participantId, net::IPv4 ip) {

    rpc::sw::AddParticipant msg;
    msg.sessionId = sessionId;
    msg.participantId = participantId;
    msg.ip = ip;
    _ws->write(msg.to_json());

    // TODO: think through + deal with situation where participant wants to receive existing streams
    //        -> probably add stream type to indicate that stream is a 'receive stream'
}

void p4sfu::ControllerSwitchConnection::addStream(unsigned sessionId, unsigned participantId,
                                                  const Stream::SendStreamConfig& stream) {

    this->_addStream(sessionId, participantId, SDP::Direction::sendonly, stream.addr.ip(),
              stream.addr.port(), stream.iceUfrag, stream.icePwd, stream.mediaType,
              stream.mainSSRC, stream.rtxSSRC);
}

void p4sfu::ControllerSwitchConnection::addStream(unsigned sessionId, unsigned participantId,
                                                  const Stream::ReceiveStreamConfig& stream) {

    this->_addStream(sessionId, participantId, SDP::Direction::recvonly, stream.addr.ip(),
              stream.addr.port(), stream.iceUfrag, stream.icePwd, stream.mediaType,
              stream.mainSSRC, stream.rtxSSRC, stream.rtcpSSRC, stream.rtcpRtxSSRC);
}

void p4sfu::ControllerSwitchConnection::_addStream(
    unsigned sessionId, unsigned participantId, SDP::Direction dir, net::IPv4 ip, std::uint16_t port,
    const std::string& iceUfrag, const std::string& icePwd, MediaType mediaType, SSRC ssrc1,
    SSRC ssrc2, SSRC rtcpSSRC, SSRC rtcpRtxSSRC) {

    rpc::sw::AddStream msg;
    msg.sessionId = sessionId;
    msg.participantId = participantId;
    msg.ip = ip;
    msg.port = port;
    msg.iceUfrag = iceUfrag;
    msg.icePwd = icePwd;
    msg.mediaType = mediaType;
    msg.mainSSRC = ssrc1;
    msg.rtxSSRC = ssrc2;
    msg.rtcpSSRC = rtcpSSRC;
    msg.rtcpRtxSSRC = rtcpRtxSSRC;
    msg.direction = dir;
    _ws->write(msg.to_json());
}

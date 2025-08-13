
#include "rpc.h"

rpc::Message::Message(std::string type)
    : type(std::move(type)) { }

const std::string rpc::sw::AddParticipant::TYPE_IDENT = "add_participant";

rpc::sw::AddParticipant::AddParticipant()
    : rpc::Message(rpc::sw::AddParticipant::TYPE_IDENT) { }

rpc::sw::AddParticipant::AddParticipant(const json::json& msg)
    : rpc::Message(rpc::sw::AddParticipant::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::sw::AddParticipant::TYPE_IDENT) {
        type = msg["type"];
    } else {
        throw std::runtime_error("rpc::sw::AddParticipant: failed parsing types");
    }

    if (msg.find("data") != msg.end()) {

        if (msg["data"].find("session_id") != msg["data"].end()) {
            sessionId = msg["data"]["session_id"];
        } else {
            throw std::runtime_error("rpc::sw::AddParticipant: failed parsing session_id");
        }

        if (msg["data"].find("participant_id") != msg["data"].end()) {
            participantId = msg["data"]["participant_id"];
        } else {
            throw std::runtime_error("rpc::sw::AddParticipant: failed parsing participant_id");
        }

        if (msg["data"].find("ip") != msg["data"].end()) {
            ip = net::IPv4{std::string{msg["data"]["ip"]}};
        } else {
            throw std::runtime_error("rpc::sw::AddParticipant: failed parsing ip");
        }

    } else {
        throw std::runtime_error("rpc::sw::AddParticipant: failed parsing data");
    }
}

std::string rpc::sw::AddParticipant::to_json() const  {

    json::json msg;
    msg["type"] = type;
    msg["data"] = {};
    msg["data"]["session_id"] = sessionId;
    msg["data"]["participant_id"] = participantId;
    msg["data"]["ip"] = ip.str();
    return msg.dump();
}

const std::string rpc::sw::AddStream::TYPE_IDENT = "add_stream";

rpc::sw::AddStream::AddStream()
    : rpc::Message(rpc::sw::AddStream::TYPE_IDENT) { }

rpc::sw::AddStream::AddStream(const json::json& msg)
    : rpc::Message(rpc::sw::AddStream::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::sw::AddStream::TYPE_IDENT) {
        type = msg["type"];
    } else {
        throw std::runtime_error("rpc::sw::AddStream: failed parsing type");
    }

    if (msg.find("data") != msg.end()) {

        if (msg["data"].find("session_id") != msg["data"].end()) {
            sessionId = msg["data"]["session_id"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing session_id");
        }

        if (msg["data"].find("participant_id") != msg["data"].end()) {
            participantId = msg["data"]["participant_id"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing participant_id");
        }

        if (msg["data"].find("ip") != msg["data"].end()) {
            ip = net::IPv4{std::string{msg["data"]["ip"]}};
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing ip");
        }

        if (msg["data"].find("port") != msg["data"].end()) {
            port = msg["data"]["port"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing port");
        }

        if (msg["data"].find("ice_ufrag") != msg["data"].end()) {
            iceUfrag = msg["data"]["ice_ufrag"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing ice_ufrag");
        }

        if (msg["data"].find("ice_pwd") != msg["data"].end()) {
            icePwd = msg["data"]["ice_pwd"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing ice_pwd");
        }

        if (msg["data"].find("main_ssrc") != msg["data"].end()) {
            mainSSRC = msg["data"]["main_ssrc"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing main_ssrc");
        }

        if (msg["data"].find("rtx_ssrc") != msg["data"].end()) {
            rtxSSRC = msg["data"]["rtx_ssrc"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing rtx_ssrc");
        }

        if (msg["data"].find("rtcp_ssrc") != msg["data"].end()) {
            rtcpSSRC = msg["data"]["rtcp_ssrc"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing rtcp_ssrc");
        }

        if (msg["data"].find("rtcp_rtx_ssrc") != msg["data"].end()) {
            rtcpRtxSSRC = msg["data"]["rtcp_rtx_ssrc"];
        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing rtcp_rtx_ssrc");
        }

        if (msg["data"].find("media_type") != msg["data"].end()) {

            if (msg["data"]["media_type"] == "audio") {
                mediaType = p4sfu::MediaType::audio;
            } else if (msg["data"]["media_type"] == "video") {
                mediaType = p4sfu::MediaType::video;
            }

        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing media_type");
        }

        if (msg["data"].find("direction") != msg["data"].end()) {

            if (msg["data"]["direction"] == "sendonly") {
                direction = SDP::Direction::sendonly;
            } else if (msg["data"]["direction"] == "recvonly") {
                direction = SDP::Direction::recvonly;
            }

        } else {
            throw std::runtime_error("rpc::sw::AddStream: failed parsing direction");
        }

    } else {
        throw std::runtime_error("rpc::sw::AddStream: failed parsing data");
    }
}

std::string rpc::sw::AddStream::to_json() const {

    json::json msg;
    msg["type"] = type;
    msg["data"] = {};
    msg["data"]["session_id"] = sessionId;
    msg["data"]["participant_id"] = participantId;
    msg["data"]["ip"] = ip.str();
    msg["data"]["port"] = port;
    msg["data"]["ice_ufrag"] = iceUfrag;
    msg["data"]["ice_pwd"] = icePwd;
    msg["data"]["main_ssrc"] = mainSSRC;
    msg["data"]["rtx_ssrc"] = rtxSSRC;
    msg["data"]["rtcp_ssrc"] = rtcpSSRC;
    msg["data"]["rtcp_rtx_ssrc"] = rtcpRtxSSRC;
    msg["data"]["media_type"] = p4sfu::mediaTypeString(mediaType);
    assert(direction == SDP::Direction::sendonly || direction == SDP::Direction::recvonly);
    msg["data"]["direction"] = (direction == SDP::Direction::sendonly ? "sendonly" : "recvonly");

    return msg.dump();
}

#pragma mark cl::Hello

const std::string rpc::cl::Hello::TYPE_IDENT = "hello";

rpc::cl::Hello::Hello()
    : rpc::Message(rpc::cl::Hello::TYPE_IDENT) { }

rpc::cl::Hello::Hello(const json::json& msg)
    : rpc::Message(rpc::cl::Hello::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::cl::Hello::TYPE_IDENT) {
        type = msg["type"];
    } else {
        throw std::runtime_error("rpc::Hello: failed parsing message");
    }

    if (msg.find("data") != msg.end()) {

        if (msg["data"].find("session_id") != msg["data"].end()) {
            sessionId = msg["data"]["session_id"];
        }

        if (msg["data"].find("participant_id") != msg["data"].end()) {
            participantId = msg["data"]["participant_id"];
        }
    }
}

std::string rpc::cl::Hello::to_json() const {

    json::json msg;
    msg["type"] = type;

    if (sessionId || participantId) {
        msg["data"] = {};

        if (sessionId)
            msg["data"]["session_id"] = *sessionId;

        if (participantId)
            msg["data"]["participant_id"] = *participantId;
    }

    return msg.dump();
}

#pragma mark cl::Offer

const std::string rpc::cl::Offer::TYPE_IDENT = "offer";

rpc::cl::Offer::Offer()
    : rpc::Message(rpc::cl::Offer::TYPE_IDENT) { }

rpc::cl::Offer::Offer(const json::json& msg)
    : rpc::Message(rpc::cl::Offer::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::cl::Offer::TYPE_IDENT) {
        type = msg["type"];
    } else {
        throw std::runtime_error("rpc::cl::Offer: failed parsing message");
    }

    if (msg.find("data") != msg.end()) {
        if (msg["data"].find("sdp") != msg["data"].end()) {
            sdp = msg["data"]["sdp"];
        } else {
            throw std::runtime_error("rpc::cl::Answer: failed parsing message: sdp missing");
        }

        if (msg["data"].find("participant_id") != msg["data"].end()) {
            participantId = msg["data"]["participant_id"];
        } else {
            throw std::runtime_error("rpc::cl::Offer: failed parsing sdp: participant_id missing");
        }
    }
}

std::string rpc::cl::Offer::to_json() const {

    json::json msg;
    msg["type"] = type;
    msg["data"] = {};
    msg["data"]["type"] = "offer";
    msg["data"]["sdp"] = sdp;

    if (participantId != 0) {
        msg["data"]["participant_id"] = participantId;
    }

    return msg.dump();
}

#pragma mark cl::Answer

const std::string rpc::cl::Answer::TYPE_IDENT = "answer";

rpc::cl::Answer::Answer()
    : rpc::Message(rpc::cl::Answer::TYPE_IDENT) { }

rpc::cl::Answer::Answer(const json::json& msg)
     : rpc::Message(rpc::cl::Answer::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::cl::Answer::TYPE_IDENT) {
        type = msg["type"];
    } else {
        throw std::runtime_error("rpc::cl::Answer: failed parsing message");
    }

    if (msg.find("data") != msg.end()) {
        if (msg["data"].find("sdp") != msg["data"].end()) {
            sdp = msg["data"]["sdp"];
        } else {
            throw std::runtime_error("rpc::cl::Answer: failed parsing sdp: sdp missing");
        }

        if (msg["data"].find("participant_id") != msg["data"].end()) {
            participantId = msg["data"]["participant_id"];
        } else {
            throw std::runtime_error("rpc::cl::Answer: failed parsing sdp: participant_id missing");
        }
    } else {
        throw std::runtime_error("rpc::cl::Answer: failed parsing sdp");
    }
}

std::string rpc::cl::Answer::to_json() const {

    json::json msg;
    msg["type"] = type;
    msg["data"] = {};
    msg["data"]["type"] = "answer";
    msg["data"]["sdp"] = sdp;

    if (participantId != 0) {
        msg["data"]["participant_id"] = participantId;
    }

    return msg.dump();
}

#pragma mark sw:Hello

const std::string rpc::sw::Hello::TYPE_IDENT = "hello";

rpc::sw::Hello::Hello()
    : rpc::Message(rpc::sw::Hello::TYPE_IDENT) { }

rpc::sw::Hello::Hello(const json::json& msg)
    : rpc::Message(rpc::sw::Hello::TYPE_IDENT) {

    if (msg.find("type") != msg.end() && msg["type"] == rpc::sw::Hello::TYPE_IDENT)
        type = msg["type"];
    else
        throw std::runtime_error("rpc::Hello: failed parsing message");

    if (msg.find("data") != msg.end() && msg["data"].find("switch_id") != msg["data"].end())
        switchId = msg["data"]["switch_id"];
}

std::string rpc::sw::Hello::to_json() const {

    json::json msg;
    msg["type"] = type;

    if (switchId.has_value()) {
        msg["data"] = {};
        msg["data"]["switch_id"] = *switchId;
    }

    return msg.dump();
}

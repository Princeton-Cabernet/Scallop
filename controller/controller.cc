
#include "controller.h"
#include "../lib/log.h"
#include "../lib/net/net.h"

#include <iostream>

p4sfu::Controller::Controller(const Config& c)
    : _config{c},
      _io{},
      _ssl{c.certFile, c.keyFile},
      _clients{_io, c.clientSidePort, _ssl.ssl},
      _switches{_io, c.switchSidePort},
      _api{_io, c.apiListenPort},
      _web{_io, c.webPort, _ssl.ssl},
      _timer{_io, 1000 /* ms */},
      _sessions{},
      _sfuConfig{c.dataPlaneIPv4, c.dataPlanePort, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS",
                 ::net::IPv4Prefix{c.limitNetwork, c.limitMask}} {

    Log::config = { .level = _config.verbose ? Log::DEBUG : Log::INFO, .printLabel = true };

    Log(Log::INFO) << "Controller: (): "
                   << "client-side-port=" << c.clientSidePort << ", "
                   << "switch-side-port=" << c.switchSidePort << ", "
                   << "api-listen-port=" << c.apiListenPort << ", "
                   << "web-port=" << c.webPort << ", "
                   << "data-plane-addr=" << c.dataPlaneIPv4 << ":" << c.dataPlanePort << ", "
                   << "limit-net=" << c.limitNetwork << "/" << (int) c.limitMask << ", "
                   << "cert-file=" << c.certFile << ", "
                   << "key-file=" << c.keyFile
                   << std::endl;

    // set up handlers for client-side connections:

    _clients.onOpen = [this](ControllerClientConnection& c) {
        _onClientOpen(c);
    };

    _clients.onHello = [this](ControllerClientConnection& c, const rpc::cl::Hello& m) {
        _onClientHello(c, m);
    };

    _clients.onNewParticipant = [this](ControllerClientConnection& c, const rpc::cl::Hello& m) {
        _onNewParticipant(c, m);
    };

    _clients.onOffer = [this](ControllerClientConnection& c, const rpc::cl::Offer& m) {
        _onClientOffer(c, m);
    };

    _clients.onAnswer = [this](ControllerClientConnection& c, const rpc::cl::Answer& m) {
        _onClientAnswer(c, m);
    };

    _clients.onClose = [this](const ClientMeta& m) {
        _onClientClose(m);
    };

    // set up handlers for switch-side connections:

    _switches.onOpen = [this](ControllerSwitchConnection& c) {
        _onSwitchOpen(c);
    };

    _switches.onHello = [this](ControllerSwitchConnection& c, const rpc::sw::Hello& m) {
        _onSwitchHello(c, m);
    };

    _switches.onNewSwitch = [this](ControllerSwitchConnection& c, const rpc::sw::Hello& m) {
        _onNewSwitch(c, m);
    };

    _switches.onClose = [this](const SwitchMeta& m) {
        _onSwitchClose(m);
    };

    // set up api handlers:

    _api.onOpen = [this](ControllerAPI::Connection& c) {
        Log(Log::INFO) << "Controller: api.onOpen" << std::endl;
    };

    _api.onGetSessions = [this](ControllerAPIMeta& m) -> json::json {
        return this->_onAPIGetSessions();
    };

    _api.onGetSession = [this](ControllerAPIMeta& m, unsigned id) -> json::json {
        return this->_onAPIGetSession(id);
    };

    _api.onClose = [this](const ControllerAPIMeta& m) {
        Log(Log::INFO) << "Controller: api.onClose" << std::endl;
    };

    // set up handler for timer:

    _timer.onTimer([this](Timer& t) {
        _onTimer(t);
    });

    // set up handlers for session manager:

    _sessions.onNewStream([this]
        (const Session& session, unsigned participantId, const Stream& stream) {
        _onNewStream(session, participantId, stream);
    });
}

int p4sfu::Controller::operator()() {

    try {
        _io.run();
        return 0;
    } catch (std::exception& e) {
        Log(Log::ERROR) << "Controller: io_context::run failed: " << e.what() << std::endl;
        return -1;
    }
}

#pragma mark client-side event handlers

void p4sfu::Controller::_onClientOpen(ControllerClientConnection& c) {

    Log(Log::INFO) << "Controller: _onClientOpen: from=" << c.remote() << ", socket started"
                   << std::endl;
}

void p4sfu::Controller::_onClientHello(ControllerClientConnection& c, const rpc::cl::Hello& m) {

    Log(Log::INFO) << "Controller: _onClientHello: sessionId=" << c.meta().sessionId
                   << ", participantId=" << c.meta().participantId << std::endl;

    c.sendHello();
    Log(Log::DEBUG) << "Controller: _onClientHello: sent hello to sessionId=" << c.meta().sessionId
                    << ", participantId=" << c.meta().participantId << std::endl;
}

void p4sfu::Controller::_onNewParticipant(ControllerClientConnection& c,
    const rpc::cl::Hello& m) {

    // add new session if session does not exist yet:
    if (!_sessions.exists(*m.sessionId)) {
        _sessions.add(*m.sessionId, _sfuConfig);
        Log(Log::INFO) << "Controller: _onNewParticipant: added new session with sessionId="
                       << *m.sessionId << std::endl;
    }

    // retrieve existing or newly created session:
    auto& session = _sessions.get(*m.sessionId);
    auto ip = c.remote().address().to_string();
    auto port = c.remote().port();

    // return with error if participant already exists
    if (session.participantExists(ip, port)) {
        Log(Log::ERROR) << "Controller: _onNewParticipant: participant at " << ip << ":"
                        << port << " already exists." << std::endl;
        return;
    }

    // add participant to session and update client meta:
    auto participantId = session.addParticipant(ip, port);
    c.meta().registered = true;
    c.meta().sessionId = *m.sessionId;
    c.meta().participantId = participantId;

    Log(Log::INFO) << "Controller: _onNewParticipant: added new participant to sessionId="
                   << session.id() << ", set participantId=" << participantId
                   << std::endl;

    c.sendHello();
    Log(Log::DEBUG) << "Controller: _onNewParticipant: sent hello, sessionId=" << c.meta().sessionId
                    << ", participantId=" << c.meta().participantId << std::endl;

    // add participant on switch:
    _switches[ALLOWED_SWITCH_ID].addParticipant(*m.sessionId, participantId, ::net::IPv4{ip});

    Log(Log::DEBUG) << "Controller: _onNewParticipant: added participant on switch: sessionId="
                    << c.meta().sessionId << ", participantId=" << c.meta().participantId
                    << std::endl;

    // send offer to new participant if there is more than one participant:
    if (session.participants().size() > 1 && session.countStreams() > 0) {

        auto offer = session.getOffer(participantId);
        //TODO: set correct participant id:
        c.sendOffer(offer, 0);

        Log(Log::INFO) << "Controller: _onNewParticipant: sessionId=" << session.id()
                       << ", participantId=" << participantId << ", sent offer" << std::endl;

        Log(Log::DEBUG) << "Controller: _onNewParticipant: offer:" << std::endl
                        << offer.formattedSummary() << std::endl;
    }
}

void p4sfu::Controller::_onClientOffer(ControllerClientConnection& c, const rpc::cl::Offer& m) {

    Log(Log::INFO) << "Controller: _onClientOffer: sessionID=" << c.meta().participantId
                   << ", participantId=" << c.meta().participantId << ", got offer" << std::endl;

    // retrieve session object for provided meta data:
    if (!_sessions.exists(c.meta().sessionId)) {
        Log(Log::ERROR) << "Controller: _onClientOffer: invalid session, id=" << c.meta().sessionId
                        << std::endl;
        return;
    }

    auto& session = _sessions[c.meta().sessionId];

    if (!session.participantExists(c.meta().participantId)) {
        Log(Log::ERROR) << "Controller: _onClientOffer: invalid participant, id="
                        << c.meta().participantId << std::endl;
        return;
    }

    // parse and process session description:
    SDP::SessionDescription offer{m.sdp};

    // add offer to session:
    session.addOffer(c.meta().participantId, offer);

    Log(Log::DEBUG) << "Controller: _onClientOffer: added offer from participant "
                    << c.meta().participantId << ":" << std::endl << offer.formattedSummary()
                    << std::endl;

    if (session.participants().size() > 1) {

        // send offers to all other participants:
        for (auto& [id, participant]: session.participants()) {

            if (id != c.meta().participantId) {

                auto forwardOffer = session.getOffer(id);
                auto& connection = _clients.get(session.id(), id);
                //TODO: set correct participant id:

                connection.sendOffer(forwardOffer, c.meta().participantId);

                Log(Log::INFO) << "Controller: _onClientOffer: sessionID=" << session.id()
                               << ", participantID=" << id << ", sent offer" << std::endl;

                Log(Log::DEBUG) << "Controller: _onClientOffer: offer sent to client " << id << ":"
                                << std::endl << forwardOffer.formattedSummary() << std::endl;
            }
        }

    } else {

        // generate answer directly if only single participant:
        auto answer = session.getAnswer(c.meta().participantId);
        c.sendAnswer(answer, c.meta().participantId);

        Log(Log::INFO) << "Controller: _onClientOffer: sessionId=" << session.id()
                       << ", participantId=" << c.meta().participantId << ", sent answer"
                       << std::endl;

        Log(Log::DEBUG) << "Controller: _onClientOffer: answer:" << std::endl
                        << answer.formattedSummary() << std::endl;
    }
}

void p4sfu::Controller::_onClientAnswer(ControllerClientConnection& c, const rpc::cl::Answer& m) {

    Log(Log::INFO) << "Controller: _onClientAnswer: sessionID=" << c.meta().participantId
                   << ", participantID=" << c.meta().participantId << ", got answer" << std::endl;

    // retrieve session object for provided meta data:
    if (!_sessions.exists(c.meta().sessionId)) {
        Log(Log::ERROR) << "Controller: _onClientAnswer: invalid session, id="
                        << c.meta().sessionId << std::endl;
        return;
    }

    auto& session = _sessions.get(c.meta().sessionId);

    if (!session.participantExists(c.meta().participantId)) {
        Log(Log::ERROR) << "Controller: _onClientAnswer: invalid participant, id="
                        << c.meta().participantId << std::endl;
        return;
    }

    // parse and process session description:
    SDP::SessionDescription answer(m.sdp);

    Log(Log::DEBUG) << "Controller: _onClientAnswer: answer from participant "
                    << c.meta().participantId << ":" << std::endl << answer.formattedSummary()
                    << std::endl;

    session.addAnswer(c.meta().participantId, answer);

    // if no participants left in awaitAnswer state, send answer to initiating participant in case
    // signaling is active (means that signaling started by new send stream being added):
    if (session.signalingActive()
        && session.particpantsInState(Participant::SignalingState::awaitAnswer).empty()) {

        auto initParticipantId = session.participantInState(Participant::SignalingState::gotOffer);
        auto fwdAnswer = session.getAnswer(initParticipantId);

        _clients.get(c.meta().sessionId, initParticipantId).sendAnswer(fwdAnswer, initParticipantId);
    }
}

void p4sfu::Controller::_onClientClose(const ClientMeta& m) {

    if (_sessions.exists(m.sessionId)) {
        auto& session = _sessions.get(m.sessionId);

        if (session.participantExists(m.participantId)) {

            session.removeParticipant(m.participantId);

            Log(Log::INFO) << "Controller: _onClientClose: removed participant, sessionId="
                           << m.sessionId << ", participantId=" << m.participantId << std::endl;
        } else {
            Log(Log::WARN) << "Controller: _onClientClose: close from non-registered participant"
                           << std::endl;
        }

    } else {
        // this can happen when client connection is dropped before hello-handshake succeeded
        Log(Log::WARN) << "Controller: _onClientClose: close from non-registered session"
                       << std::endl;
    }
}

#pragma mark switch-side event handlers

void p4sfu::Controller::_onSwitchOpen(ControllerSwitchConnection& c) {

    Log(Log::INFO) << "Controller: _onSwitchOpen: from=" << c.remote() << ", socket started"
                   << std::endl;
}

void p4sfu::Controller::_onSwitchHello(ControllerSwitchConnection& c, const rpc::sw::Hello& m) {

    Log(Log::INFO) << "Controller: _onSwitchHello" << std::endl;
}

void p4sfu::Controller::_onNewSwitch(ControllerSwitchConnection& c, const rpc::sw::Hello& m) {

    if (_switches.countRegistered()) {
        Log(Log::ERROR) << "Controller: _onNewSwitch: only single switch supported" << std::endl;
        return;
    }

    if (*m.switchId != ALLOWED_SWITCH_ID) {
        Log(Log::ERROR) << "Controller: _onNewSwitch: only switch id=0 allowed" << std::endl;
        return;
    }

    if (_switches.exists(*m.switchId)) {
        Log(Log::ERROR) << "Controller: _onNewSwitch: switch id=" << *m.switchId
                        << " already exists" << std::endl;
        return;
    }

    c.meta().registered = true;
    c.meta().id = *m.switchId;
    c.sendHello();

    Log(Log::INFO) << "Controller: _onNewSwitch: new switch registered, id=" << c.meta().id
                   << std::endl;
}

void p4sfu::Controller::_onSwitchClose(const SwitchMeta& m) {

    _switches.remove(m.id);
    Log(Log::INFO) << "Controller: _onSwitchClose: removed switch with id=" << m.id << std::endl;
}

json::json p4sfu::Controller::_onAPIGetSessions() {

    Log(Log::INFO) << "Controller: _onAPIGetSessions" << std::endl;

    json::json j;
    j["type"] = "sessions";
    j["data"] = {};
    j["data"]["sessions"] = json::json::array();

    for (auto& [id, session]: _sessions.sessions()) {
        j["data"]["sessions"].push_back({
            {"id", session.id()},
            {"participants", session.participants().size()},
            {"streams", session.countStreams()}
        });
    }

    return j;
}

json::json p4sfu::Controller::_onAPIGetSession(unsigned id) {

    Log(Log::INFO) << "Controller: _onGetSession: id=" << id << std::endl;

    json::json j;

    if (!_sessions.exists(id)) {
        j["type"] = "error";
        j["data"] = {};
        j["data"]["msg"] = "session with id " + std::to_string(id) + " does not exist.";
    } else {
        auto& s = _sessions.get(id);
        j["type"] = "session";
        j["data"] = {};
        j["data"]["id"] = s.id();
        j["data"]["participants"] = json::json::array();

        for (const auto& [pId, p]: s.participants()) {

            auto pObj = json::json::object();
            pObj["id"] = pId;
            pObj["sendStreams"] = json::json::array();
            pObj["receiveStreams"] = json::json::array();

            for (const auto& stream: p.outgoingStreams()) {
                auto streamObj = json::json::object();
                auto config = stream.dataPlaneConfig<Stream::SendStreamConfig>();
                streamObj["ip"] = config.addr.ip().str();
                streamObj["port"] = config.addr.port();
                streamObj["ssrc"] = config.mainSSRC;
                streamObj["rtxSsrc"] = config.rtxSSRC;
                pObj["sendStreams"].push_back(streamObj);
            }

            for (const auto& stream: p.incomingStreams()) {
                auto streamObj = json::json::object();
                auto config = stream.dataPlaneConfig<Stream::ReceiveStreamConfig>();
                streamObj["ip"] = config.addr.ip().str();
                streamObj["port"] = config.addr.port();
                streamObj["ssrc"] = config.mainSSRC;
                streamObj["rtxSsrc"] = config.rtxSSRC;
                streamObj["rtcpSsrc"] = config.rtcpSSRC;
                streamObj["rtcpRtxSsrc"] = config.rtcpRtxSSRC;
                pObj["receiveStreams"].push_back(streamObj);
            }

            j["data"]["participants"].push_back(pObj);
        }
    }

    return j;
}

void p4sfu::Controller::_onNewStream(const p4sfu::Session& session, unsigned participantId,
                                     const p4sfu::Stream& stream) {

    Log(Log::INFO) << "Controller: _onNewStream: sessionId=" << session.id() << std::endl;

    if (!stream.description().direction) {
        Log(Log::ERROR) << "Controller: _onNewStream: stream has no direction" << std::endl;
        return;
    }

    if (std::holds_alternative<Stream::SendStreamConfig>(stream.dataPlaneConfig())) {

        // install send stream:
        auto sendStream = std::get<Stream::SendStreamConfig>(stream.dataPlaneConfig());
        _switches[ALLOWED_SWITCH_ID].addStream(session.id(), participantId, sendStream);

        Log(Log::INFO) << "Controller: _onNewStream: added send stream on switch: "
                       << "addr=" << sendStream.addr << ", ssrc=[" << sendStream.mainSSRC
                       << (sendStream.rtxSSRC ? (","+std::to_string(sendStream.rtxSSRC)) : "")
                       << "]" << std::endl;

    } else if (std::holds_alternative<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig())) {

        // install receive stream:
        auto receiveStream = std::get<Stream::ReceiveStreamConfig>(stream.dataPlaneConfig());
        _switches[ALLOWED_SWITCH_ID].addStream(session.id(), participantId, receiveStream);

        Log(Log::INFO) << "Controller: _onNewStream: added receive stream on switch: "
                       << "addr=" << receiveStream.addr << ", ssrc=[" << receiveStream.mainSSRC
                       << (receiveStream.rtxSSRC ? (","+std::to_string(receiveStream.rtxSSRC)) : "")
                       << "], rtcp-ssrc=[" << std::to_string(receiveStream.rtcpSSRC)
                       << (receiveStream.rtcpRtxSSRC ?
                             (","+std::to_string(receiveStream.rtcpRtxSSRC)) : "") << "]"
                       << std::endl;
    }
}

void p4sfu::Controller::_onTimer(Timer& t) {

    // std::cout << "Controller: _onTimer(): count=" << t.count() << std::endl;
}

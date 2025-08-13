#include "controller_client_interface.h"

#include "util.h"
#include "log.h"


p4sfu::ControllerClientInterface::ControllerClientInterface(asio::io_context& io,
    unsigned short port, asio::ssl::context& ssl)
    : _server(io, port, ssl) {

    _server.onOpen([this](auto&&... args) {
        _onClientOpen(std::forward<decltype(args)>(args)...);
    });

    _server.onMessage([this](auto&&... args) {
        _onClientMessage(std::forward<decltype(args)>(args)...);
    });

    _server.onClose([this](auto&&... args) {
        _onClientClose(std::forward<decltype(args)>(args)...);
    });
}

p4sfu::ControllerClientConnection&
p4sfu::ControllerClientInterface::get(unsigned sessionId, unsigned participantId) {

    auto conn_it = std::find_if(_connections.begin(), _connections.end(),
                                [sessionId, participantId](auto &c) -> bool {
                                    return c.second.meta().sessionId == sessionId
                                           && c.second.meta().participantId == participantId;
                                });

    if (conn_it != _connections.end()) {
        return conn_it->second;
    } else {
        throw std::logic_error("ControllerClientInterface: get: participant does not exist");
    }
}

void p4sfu::ControllerClientInterface::_onClientOpen(SecureWebSocketSession<ClientMeta>& s) {

    s.start();

    auto [it, success] = _connections.emplace(s.remote(), ControllerClientConnection{s});

    if (success) {
        onOpen(it->second);

        Log(Log::DEBUG) << "ControllerClientInterface: _onClientOpen: added connection"
                        << std::endl;

    } else {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientOpen: failed adding new connection"
                        << std::endl;
    }
}

void p4sfu::ControllerClientInterface::_onClientMessage(SecureWebSocketSession<ClientMeta>& s,
    const char* buf, std::size_t len) {

    Log(Log::DEBUG) << "ControllerClientInterface: _onClientMessage: sessionId="
                    << s.meta().sessionId << ", participantId=" << s.meta().participantId
                    << ", msg=" << util::bufferString(buf, len) << std::endl;

    auto msg = _parseClientMessage(buf, len);

    if (msg) {

        if (s.meta().registered) {

            if ((*msg)["type"] == rpc::cl::Hello::TYPE_IDENT) {
                _onClientHello(s, rpc::cl::Hello{*msg});
            } else if ((*msg)["type"] == rpc::cl::Offer::TYPE_IDENT) {
                _onClientOffer(s, rpc::cl::Offer{*msg});
            } else if ((*msg)["type"] == rpc::cl::Answer::TYPE_IDENT) {
                _onClientAnswer(s, rpc::cl::Answer{*msg});
            }

        } else { // only allow hello message from non-registered clients
            if ((*msg)["type"] == rpc::cl::Hello::TYPE_IDENT) {
                _onClientHello(s, rpc::cl::Hello{*msg});
            } else {
                Log(Log::ERROR) << "ControllerClientInterface: _onClientMessage: msg="
                    << util::bufferString(buf, len)
                    << ": non-hello message from not-registered client" << std::endl;
            }
        }
    }
}

void p4sfu::ControllerClientInterface::_onClientHello(SecureWebSocketSession<ClientMeta>& s,
    const rpc::cl::Hello& m) {

    Log(Log::DEBUG) << "ControllerClientInterface: _onClientHello: sessionId=" << s.meta().sessionId
                    << ", participantId=" << s.meta().participantId << std::endl;

    // find connection using remote endpoint:

    auto conn_it = _connections.find(s.remote());

    if (conn_it == _connections.end()) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientHello: "
                        << "hello from unregistered client, aborting ..." << std::endl;
        return;
    }

    // invoke onNewParticipant event if client is not yet registered

    if (!s.meta().registered) {

        try {
            onNewParticipant(conn_it->second, m);
        } catch (std::bad_function_call& e) {
            Log(Log::ERROR) << "ControllerClientInterface: _onClientHello: "
                            << "no onNewParticipant event handler set, aborting ..." << std::endl;
        }
    } else {

        if (m.sessionId) {

            try {
                onHello(conn_it->second, m);
            } catch (std::bad_function_call &e) {
                Log(Log::ERROR) << "ControllerClientInterface: _onClientHello: "
                                << "no onHello event handler set, aborting ..." << std::endl;
            }
        }
    }
}

void p4sfu::ControllerClientInterface::_onClientOffer(SecureWebSocketSession<ClientMeta>& s,
    const rpc::cl::Offer& m) {

    Log(Log::DEBUG) << "ControllerClientInterface: _onClientOffer: sessionId=" << s.meta().sessionId
                    << ", participantId=" << s.meta().participantId << std::endl;

    auto conn_it = _connections.find(s.remote());

    if (conn_it == _connections.end() || !s.meta().registered) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientOffer: "
                        << "offer from unknown or unregistered client, aborting ..." << std::endl;
        return;
    }

    try {
        onOffer(conn_it->second, m);
    } catch (std::bad_function_call &e) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientOffer: "
                        << "no onOffer event handler set, aborting ..." << std::endl;
    }
}

void  p4sfu::ControllerClientInterface::_onClientAnswer(SecureWebSocketSession<ClientMeta>& s,
    const rpc::cl::Answer& m) {

    Log(Log::DEBUG) << "ControllerClientInterface: _onClientAnswer: sessionId=" << s.meta().sessionId
                    << ", participantId=" << s.meta().participantId << std::endl;

    auto conn_it = _connections.find(s.remote());

    if (conn_it == _connections.end() || !s.meta().registered) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientAnswer: "
                        << "answer from unknown or unregistered client, aborting ..." << std::endl;
        return;
    }

    try {
        onAnswer(conn_it->second, m);
    } catch (std::bad_function_call &e) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientAnswer: "
                        << "no onAnswer event handler set, aborting ..." << std::endl;
    }
}

void p4sfu::ControllerClientInterface::_onClientClose(SecureWebSocketSession<ClientMeta>& s) {

    try {
        onClose(s.meta());
    } catch (std::bad_function_call &e) {
        Log(Log::ERROR) << "ControllerClientInterface: _onClientClose: "
                        << "no onClose event handler set" << std::endl;
    }
}

std::optional<json::json> p4sfu::ControllerClientInterface::_parseClientMessage(
    const char* buf, std::size_t len) {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()
            && (msg["type"] == "hello") || (msg["type"] == "offer") || (msg["type"] == "answer")) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) {
        Log(Log::ERROR)
            << "ControllerClientInterface: _parseClientMessage: failed parsing message: what="
            << e.what() << ", msg=" << util::hexString(buf, len) << std::endl;
    }

    return std::nullopt;
}

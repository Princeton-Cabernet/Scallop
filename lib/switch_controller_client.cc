
#include "switch_controller_client.h"

#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std::placeholders;
namespace json = nlohmann;

p4sfu::SwitchControllerClient::SwitchControllerClient(asio::io_context& io)
    : _ws(io) {

    _ws.onOpen(std::bind(&SwitchControllerClient::_onOpen, this, _1));
    _ws.onMessage(std::bind(&SwitchControllerClient::_onMessage, this, _1, _2, _3));
    _ws.onClose(std::bind(&SwitchControllerClient::_onClose, this, _1));
}

void p4sfu::SwitchControllerClient::onOpen(OnOpenHandler&& f) {

    _onOpenHandler = std::move(f);
}

void p4sfu::SwitchControllerClient::onHello(OnHelloHandler&& f) {

    _onHelloHandler = std::move(f);
}

void p4sfu::SwitchControllerClient::onAddParticipant(OnAddParticipantHandler&& f) {

    _onAddParticipantHandler = f;
}

void p4sfu::SwitchControllerClient::onAddStream(OnAddStreamHandler&& f) {

    _onAddStreamHandler = f;
}

void p4sfu::SwitchControllerClient::onClose(OnCloseHandler&& f) {

    _onCloseHandler = std::move(f);
}

void p4sfu::SwitchControllerClient::connect(const std::string& ip, unsigned short port) {

    _ws.connect(ip, port);
}

void p4sfu::SwitchControllerClient::sendHello() {

    _ws.write(rpc::sw::Hello{}.to_json());
}

void p4sfu::SwitchControllerClient::sendHello(unsigned switchId) {

    rpc::sw::Hello hello;
    hello.switchId = switchId;
    _ws.write(hello.to_json());
}

void p4sfu::SwitchControllerClient::_onOpen(WebSocketClient& c) {

    if (_onOpenHandler) (*_onOpenHandler)(*this);
}

void p4sfu::SwitchControllerClient::_onMessage(WebSocketClient& c, char* buf, std::size_t len) {

    json::json msg = json::json::parse(buf, buf + len);

    if (msg.find("type") != msg.end()) {

        if (msg["type"] == "hello") {

            if (_onHelloHandler) {
                (*_onHelloHandler)(*this, rpc::sw::Hello{msg});
            }

        } else if (msg["type"] == "add_participant") {

            if (_onAddParticipantHandler) {
                (*_onAddParticipantHandler)(*this, rpc::sw::AddParticipant{msg});
            }

        } else if (msg["type"] == "add_stream") {

            if (_onAddStreamHandler) {
                (*_onAddStreamHandler)(*this, rpc::sw::AddStream{msg});
            }

        } else {
            throw std::runtime_error("invalid message type");
        }
    } else {
        throw std::runtime_error("message type not found");
    }
}

void p4sfu::SwitchControllerClient::_onClose(WebSocketClient& c) {

    if (_onCloseHandler) (*_onCloseHandler)(*this);
}

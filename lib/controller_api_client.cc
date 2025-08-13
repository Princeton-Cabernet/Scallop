
#include "controller_api_client.h"

#include <iostream>

p4sfu::ControllerAPIClient::ControllerAPIClient(boost::asio::io_context& io)
    : APIClient{io} { }

void p4sfu::ControllerAPIClient::runCLICommand(const std::vector<std::string>& cmd) {

    // show sessions
    // show session <id>

    if (cmd[0] == "show" || cmd[0] == "s") {
        if (cmd.size() == 2 || cmd.size() == 3) {
            if (cmd[1] == "sessions" || cmd[1] == "ses") {
                getSessions();
            } else if (cmd[1] == "session" || cmd[1] == "se") {
                if (cmd.size() == 3) {
                    try {
                        getSession(std::stoul(cmd[2]));
                    } catch (std::exception &e) {
                        std::cerr << "invalid session id: " << e.what() << std::endl;
                    }
                } else {
                    throw std::invalid_argument{"invalid command"};
                }
            } else {
                throw std::invalid_argument{"invalid command"};
            }

        } else {
            throw std::invalid_argument{"invalid command"};
        }
    } else {
        throw std::invalid_argument{"invalid command"};
    }
}

void p4sfu::ControllerAPIClient::getSessions() {

    json::json msg;
    msg["type"] = "get-sessions";
    _ws.write(msg.dump());
}

void p4sfu::ControllerAPIClient::getSession(unsigned id) {

    json::json msg;
    msg["type"] = "get-session";
    msg["data"] = {};
    msg["data"]["id"] = id;
    _ws.write(msg.dump());
}

bool p4sfu::ControllerAPIClient::isConnected() const {
    return _ws.isConnected();
}

void p4sfu::ControllerAPIClient::disconnect() {

    _ws.close();
}

void  p4sfu::ControllerAPIClient::_onMessage(WebSocketClient& c, char* buf, std::size_t len) {

    if (len > 0) {
        auto msg = _parseMessage(buf, len);

        if (msg) {
            if ((*msg)["type"] == "error") {
                std::cerr << "error: " << to_string((*msg)["data"]["msg"]) << std::endl;
            } else {
                std::cout << (*msg)["data"].dump(4);
            }
        } else {
            std::cerr << "failed to parse message" << std::endl;
        }
    } else {
        std::cerr << "received 0 bytes" << std::endl;
    }

    _ws.close();
}

std::optional<json::json> p4sfu::ControllerAPIClient::_parseMessage(const char* buf, std::size_t len) {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()
            && (msg["type"] == "session") || (msg["type"] == "sessions")
                || (msg["type"] == "error")) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) {
        std::cerr << "failed to parse message: " << e.what() << std::endl;
    }

    return std::nullopt;
}
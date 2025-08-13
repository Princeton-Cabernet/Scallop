
#include "switch_api_client.h"

#include <iostream>

p4sfu::SwitchAPIClient::SwitchAPIClient(boost::asio::io_context& io)
    : APIClient{io} { }

void p4sfu::SwitchAPIClient::runCLICommand(const std::vector<std::string>& cmd) {

    // show,s
    //   streams,str
    // set
    //   decode-target,dt <session-id> <ssrc> <participant-id> <decode-target>

    if (cmd[0] == "show" || cmd[0] == "s") {
        if (cmd.size() == 2) {
            if (cmd[1] == "streams" || cmd[1] == "str") {
                getStreams();
            } else {
                throw std::invalid_argument{"invalid command"};
            }

        } else {
            throw std::invalid_argument{"invalid command"};
        }
    } else if (cmd[0] == "set") {
        if (cmd.size() == 6) {
            if (cmd[1] == "decode-target" || cmd[1] == "dt") {

                setDecodeTarget(std::stoi(cmd[2]), std::stoul(cmd[3]), std::stoi(cmd[4]),
                                std::stoi(cmd[5]));
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

void p4sfu::SwitchAPIClient::getStreams() {

    json::json msg;
    msg["type"] = "get-streams";
    _ws.write(msg.dump());
}

void p4sfu::SwitchAPIClient::setDecodeTarget(unsigned sessionId, SSRC ssrc, unsigned participantId,
                                             unsigned decodeTarget) {

    json::json msg;
    msg["type"] = "set-decode-target";
    msg["args"] = {
        {"session-id", sessionId},
        {"ssrc", ssrc},
        {"participant-id", participantId},
        {"decode-target", decodeTarget}
    };

    _ws.write(msg.dump());
}

void p4sfu::SwitchAPIClient::_onMessage(WebSocketClient& c, char* buf, std::size_t len) {

    std::optional<json::json> msg;

    std::memcpy(_rxBuf + _rxBufLen, buf, len);
    _rxBufLen += len;

    msg = _parseMessage(_rxBuf, _rxBufLen);

    if (msg) {
        _rxBufLen = 0;

        if ((*msg)["type"] == "ok") {
            std::cout << "ok" << std::endl;
        } else if ((*msg)["type"] == "error") {
            std::cerr << (*msg).dump(4) << std::endl;
        } else {
            std::cout << (*msg)["data"].dump(4) << std::endl;
        }

        _ws.close();
    }
}

std::optional<json::json> p4sfu::SwitchAPIClient::_parseMessage(const char* buf, std::size_t len) {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) { }

    return std::nullopt;
}


#include "controller_api.h"
#include "util.h"

const std::string p4sfu::ControllerAPI::CLASS = "ControllerAPI";

p4sfu::ControllerAPI::ControllerAPI(asio::io_context &io, unsigned short port)
    : API{io, port} { }

void p4sfu::ControllerAPI::_onMessage(WebSocketSession<ControllerAPIMeta>& s, const char* buf,
                                  std::size_t len) {

    auto msg = _parseMessage(buf, len);

    if (msg) {
        if ((*msg)["type"] == "get-sessions") {
            auto response = onGetSessions(s.meta());
            s.write(response.dump());
        } else if ((*msg)["type"] == "get-session") {
            auto response = onGetSession(s.meta(), (*msg)["data"]["id"]);
            s.write(response.dump());
        }
    }
}

std::optional<json::json> p4sfu::ControllerAPI::_parseMessage(const char* buf, std::size_t len) const {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()
            && (    msg["type"] == "get-session")
            || (msg["type"] == "get-sessions")) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) {
        Log(Log::ERROR)
                << CLASS << ": _parseClientMessage: failed parsing message: what="
                << e.what() << ", msg=" << util::hexString(buf, len) << std::endl;
    }

    return std::nullopt;
}

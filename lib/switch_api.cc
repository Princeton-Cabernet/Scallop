
#include "switch_api.h"
#include "util.h"

const std::string p4sfu::SwitchAPI::CLASS = "SwitchAPI";

p4sfu::SwitchAPI::SwitchAPI(asio::io_context &io, unsigned short port)
    : API{io, port} { }

void p4sfu::SwitchAPI::_onMessage(WebSocketSession<SwitchAPIMeta>& s, const char* buf,
                                  std::size_t len) {

    auto msg = _parseMessage(buf, len);

    if (msg) {
        if ((*msg)["type"] == "get-streams") {
            auto response = onGetStreams(s.meta());
            s.write(response.dump());
        } else if ((*msg)["type"] == "set-decode-target") {
            auto args = (*msg)["args"];
            auto response = onSetDecodeTarget(s.meta(), args["session-id"], args["ssrc"],
                                              args["participant-id"], args["decode-target"]);

            s.write(response.dump());
        }
    }
}

std::optional<json::json> p4sfu::SwitchAPI::_parseMessage(const char* buf, std::size_t len) const {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()
            && ((msg["type"] == "get-streams")
            ||  (msg["type"] == "set-decode-target"))) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) {
        Log(Log::ERROR)
                << "SwitchAPI: _parseMessage: failed parsing message: what="
                << e.what() << ", msg=" << util::bufferString(buf, len) << std::endl;
    }

    return std::nullopt;
}

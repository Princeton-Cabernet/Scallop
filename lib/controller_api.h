#ifndef P4SFU_CONTROLLER_API_H
#define P4SFU_CONTROLLER_API_H

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "api.h"

namespace json = nlohmann;
using namespace boost;

namespace p4sfu {

    struct ControllerAPIMeta { };

    class ControllerAPI : public API<ControllerAPIMeta> {
        static const std::string CLASS;
    public:
        explicit ControllerAPI(asio::io_context &io, unsigned short port);
        std::function<json::json (ControllerAPIMeta&)> onGetSessions;
        std::function<json::json (ControllerAPIMeta&, unsigned)> onGetSession;

    private:
        void _onMessage(WebSocketSession<ControllerAPIMeta>& s, const char* buf, std::size_t len) override;
        std::optional<json::json> _parseMessage(const char* buf, std::size_t len) const;
    };
}

#endif

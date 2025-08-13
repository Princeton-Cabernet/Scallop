#ifndef P4SFU_SWITCH_API_H
#define P4SFU_SWITCH_API_H

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "api.h"

namespace json = nlohmann;
using namespace boost;

namespace p4sfu {

    struct SwitchAPIMeta { };

    class SwitchAPI : public API<SwitchAPIMeta> {
        static const std::string CLASS;
    public:
        explicit SwitchAPI(asio::io_context &io, unsigned short port);
        std::function<json::json (SwitchAPIMeta&)> onGetStreams;
        std::function<json::json (SwitchAPIMeta&, unsigned, unsigned, unsigned, unsigned)>
                onSetDecodeTarget;

    private:
        void _onMessage(WebSocketSession<SwitchAPIMeta>& s, const char* buf, std::size_t len) override;
        std::optional<json::json> _parseMessage(const char* buf, std::size_t len) const;
    };
}

#endif

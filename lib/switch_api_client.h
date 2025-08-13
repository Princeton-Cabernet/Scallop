
#ifndef P4SFU_SWITCH_API_CLIENT_H
#define P4SFU_SWITCH_API_CLIENT_H

#include "api_client.h"
#include "p4sfu.h"

namespace json = nlohmann;

namespace p4sfu {
    class SwitchAPIClient : public APIClient {
    public:
        explicit SwitchAPIClient(boost::asio::io_context& io);
        void runCLICommand(const std::vector<std::string>& cmd);
        void getStreams();
        void setDecodeTarget(unsigned sessionId, SSRC ssrc, unsigned participantId,
                             unsigned decodeTarget);

    private:
        void _onMessage(WebSocketClient& c, char* buf, std::size_t len) override;
        std::optional<json::json> _parseMessage(const char* buf, std::size_t len) override;
        char _rxBuf[16384] = {0};
        unsigned _rxBufLen = 0;
    };
}

#endif

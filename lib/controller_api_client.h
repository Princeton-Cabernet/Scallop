
#ifndef P4SFU_CONTROLLER_API_CLIENT_H
#define P4SFU_CONTROLLER_API_CLIENT_H

#include "api_client.h"

namespace json = nlohmann;

namespace p4sfu {
    class ControllerAPIClient : public APIClient  {
    public:
        explicit ControllerAPIClient(boost::asio::io_context& io);
        void runCLICommand(const std::vector<std::string>& cmd);
        void getSessions();
        void getSession(unsigned id);
        bool isConnected() const;
        void disconnect();

    private:
        void _onMessage(WebSocketClient& c, char* buf, std::size_t len) override;
        std::optional<json::json> _parseMessage(const char* buf, std::size_t len) override;
    };
}

#endif

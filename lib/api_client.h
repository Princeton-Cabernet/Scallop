
#ifndef P4SFU_API_CLIENT_H
#define P4SFU_API_CLIENT_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include "net/web_socket_client.h"

namespace json = nlohmann;

namespace p4sfu {

    class APIClient {
    public:
        APIClient() = delete;
        APIClient(const APIClient&) = delete;
        APIClient& operator=(const APIClient&) = delete;

        explicit APIClient(boost::asio::io_context& io);
        void open(const std::string& host, unsigned short port);
        bool isOpen() const;
        void close();
        virtual ~APIClient() = default;

    protected:
        virtual void _onMessage(WebSocketClient& c, char* buf, std::size_t len) = 0;
        virtual std::optional<json::json> _parseMessage(const char* buf, std::size_t len) = 0;

        boost::asio::io_context& _io;
        WebSocketClient _ws;
    };
}

#endif

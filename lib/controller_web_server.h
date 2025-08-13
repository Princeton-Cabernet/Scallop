
#ifndef P4SFU_CONTROLLER_WEB_SERVER_H
#define P4SFU_CONTROLLER_WEB_SERVER_H

#include "net/http_server.h"
#include <nlohmann/json.hpp>
#include "inja/inja.hpp"

namespace json = nlohmann;
using namespace boost;

namespace p4sfu {

    class ControllerWebServer {

        static const std::string CLASS;

    public:
        ControllerWebServer(asio::io_context& io, std::uint16_t port, asio::ssl::context& ssl);

    private:

        void _serveSFUApp(HTTPServer::Session& s, const HTTPServer::Target& target);

        HTTPServer _server;
        inja::Environment _inja;
    };
}

#endif

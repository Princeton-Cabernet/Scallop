#ifndef P4SFU_CONTROLLER_CLIENT_INTERFACE_H
#define P4SFU_CONTROLLER_CLIENT_INTERFACE_H

#include <boost/asio.hpp>
#include <unordered_map>

#include "net/secure_web_socket_server.h"
#include "net/secure_web_socket_session.h"
#include "controller_client_connection.h"
#include <nlohmann/json.hpp>
#include "rpc.h"

namespace json = nlohmann;

using namespace boost;

namespace p4sfu {

    class ControllerClientInterface {

    public:
        ControllerClientInterface(asio::io_context& io, unsigned short port,
                                  asio::ssl::context& ssl);

        std::function<void (ControllerClientConnection&)> onOpen;
        std::function<void (ControllerClientConnection&, const rpc::cl::Hello&)> onHello;
        std::function<void (ControllerClientConnection&, const rpc::cl::Hello&)> onNewParticipant;
        std::function<void (ControllerClientConnection&, const rpc::cl::Offer&)> onOffer;
        std::function<void (ControllerClientConnection&, const rpc::cl::Answer&)> onAnswer;
        std::function<void (const ClientMeta&)> onClose;

        ControllerClientConnection& get(unsigned sessionId, unsigned participantId);

    private:
        void _onClientOpen(SecureWebSocketSession<ClientMeta>& s);
        void _onClientMessage(SecureWebSocketSession<ClientMeta>& s, const char* buf, std::size_t len);
        void _onClientHello(SecureWebSocketSession<ClientMeta>& s, const rpc::cl::Hello& m);
        void _onClientOffer(SecureWebSocketSession<ClientMeta>& s, const rpc::cl::Offer& m);
        void _onClientAnswer(SecureWebSocketSession<ClientMeta>& s, const rpc::cl::Answer& m);
        void _onClientClose(SecureWebSocketSession<ClientMeta>& s);

        static std::optional<json::json> _parseClientMessage(const char* buf, std::size_t len);

        SecureWebSocketServer<SecureWebSocketSession<ClientMeta>> _server;
        // WebSocketServer<WebSocketSession<ClientMeta>> _server;
        std::map<asio::ip::tcp::endpoint, ControllerClientConnection> _connections;
    };
}

#endif

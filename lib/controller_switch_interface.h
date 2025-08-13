#ifndef P4SFU_CONTROLLER_SWITCH_INTERFACE_H
#define P4SFU_CONTROLLER_SWITCH_INTERFACE_H

#include <boost/asio.hpp>
#include <unordered_map>

#include "net/web_socket_server.h"
#include "controller_switch_connection.h"
#include <nlohmann/json.hpp>
#include "rpc.h"

namespace json = nlohmann;

using namespace boost;

namespace p4sfu {

    class ControllerSwitchInterface {

        static const std::string CLASS;

    public:
        ControllerSwitchInterface(asio::io_context& io, unsigned short port);

        std::function<void (ControllerSwitchConnection&)> onOpen;
        std::function<void (ControllerSwitchConnection&, const rpc::sw::Hello&)> onNewSwitch;
        std::function<void (ControllerSwitchConnection&, const rpc::sw::Hello&)> onHello;
        std::function<void (const SwitchMeta&)> onClose;

        [[nodiscard]] unsigned countRegistered();
        [[nodiscard]] bool exists(unsigned switchId);
        ControllerSwitchConnection& get(unsigned switchId);
        ControllerSwitchConnection& operator[](unsigned switchId);
        void remove(unsigned switchId);

    private:
        void _onSwitchOpen(WebSocketSession<SwitchMeta>& s);
        void _onSwitchMessage(WebSocketSession<SwitchMeta>& s, const char* buf, std::size_t len);
        void _onSwitchHello(WebSocketSession<SwitchMeta>& s, const rpc::sw::Hello& m);
        void _onSwitchClose(WebSocketSession<SwitchMeta>& s);

        static std::optional<json::json> _parseSwitchMessage(const char* buf, std::size_t len);

        std::optional<std::map<asio::ip::tcp::endpoint, ControllerSwitchConnection>::iterator>
            _connectionById(unsigned switchId, bool reg = false);

        WebSocketServer<WebSocketSession<SwitchMeta>> _server;
        std::map<asio::ip::tcp::endpoint, ControllerSwitchConnection> _connections;
    };
}

#endif

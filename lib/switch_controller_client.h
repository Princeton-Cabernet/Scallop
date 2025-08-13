
#ifndef P4SFU_SWITCH_CONTROLLER_CLIENT_H
#define P4SFU_SWITCH_CONTROLLER_CLIENT_H

#include "net/web_socket_client.h"
#include "rpc.h"
#include <boost/asio.hpp>

using namespace boost;

namespace p4sfu {

    /// Switch-side client for switch-controller communication.
    ///
    /// uses WebSocketClient internally

    class SwitchControllerClient {

    public:

        typedef std::function<void (SwitchControllerClient&)> OnOpenHandler;
        typedef std::function<void (SwitchControllerClient&, const rpc::sw::Hello&)> OnHelloHandler;
        typedef std::function<void (SwitchControllerClient&,
                const rpc::sw::AddParticipant&)> OnAddParticipantHandler;
        typedef std::function<void (SwitchControllerClient&,
                const rpc::sw::AddStream&)> OnAddStreamHandler;
        typedef std::function<void (SwitchControllerClient&)> OnCloseHandler;

        explicit SwitchControllerClient(asio::io_context& io);

        void onOpen(OnOpenHandler&& f);
        void onHello(OnHelloHandler&& f);
        void onAddParticipant(OnAddParticipantHandler&& f);
        void onAddStream(OnAddStreamHandler&& f);
        void onClose(OnCloseHandler&& f);

        void connect(const std::string& ip, unsigned short port);

        void sendHello();
        void sendHello(unsigned switchId);

    private:

        void _onOpen(WebSocketClient& c);
        void _onMessage(WebSocketClient& c, char* buf, std::size_t len);
        void _onClose(WebSocketClient& c);

        WebSocketClient _ws;

        std::optional<OnOpenHandler> _onOpenHandler = std::nullopt;
        std::optional<OnHelloHandler> _onHelloHandler = std::nullopt;
        std::optional<OnAddParticipantHandler> _onAddParticipantHandler = std::nullopt;
        std::optional<OnAddStreamHandler> _onAddStreamHandler = std::nullopt;
        std::optional<OnCloseHandler> _onCloseHandler = std::nullopt;
    };
}

#endif

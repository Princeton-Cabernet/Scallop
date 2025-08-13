#ifndef P4SFU_CONTROLLER_SWITCH_CONNECTION_H
#define P4SFU_CONTROLLER_SWITCH_CONNECTION_H

#include "net/net.h"
#include "stream.h"
#include "net/web_socket_session.h"

namespace p4sfu {

    struct SwitchMeta {
        bool registered = false;
        unsigned id = 0;

        bool operator==(const SwitchMeta& other) const;
        bool operator<(const SwitchMeta& other) const;
    };

    class ControllerSwitchConnection {

    public:
        ControllerSwitchConnection() = delete;
        ControllerSwitchConnection(const ControllerSwitchConnection&) = default;
        ControllerSwitchConnection& operator=(const ControllerSwitchConnection&) = default;

        explicit ControllerSwitchConnection(WebSocketSession<SwitchMeta>& s);

        void sendHello();

        //! adds a participant to the data plane
        void addParticipant(unsigned sessionId, unsigned participantId, net::IPv4 ip);

        //! adds a send stream to the data plane
        void addStream(unsigned sessionId, unsigned participantId,
                       const Stream::SendStreamConfig& stream);

        //! adds a receive stream to the data plane
        void addStream(unsigned sessionId, unsigned participantId,
                       const Stream::ReceiveStreamConfig& stream);

        inline const asio::ip::tcp::endpoint& remote() const {
            return _ws->remote();
        }

        inline SwitchMeta& meta() {
            return _ws->meta();
        }

    private:

        void _addStream(unsigned sessionId, unsigned particpantId, SDP::Direction dir, net::IPv4 ip,
                        std::uint16_t port, const std::string& iceUfrag, const std::string& icePwd,
                        MediaType type, SSRC ssrc1 = 0, SSRC ssrc2 = 0, SSRC rtcpSsrc = 0,
                        SSRC rtcpRtxSsrc = 0);

        std::shared_ptr<WebSocketSession<SwitchMeta>> _ws;
    };
}

#endif
#ifndef P4SFU_CONTROLLER_CLIENT_CONNECTION_H
#define P4SFU_CONTROLLER_CLIENT_CONNECTION_H

#include "net/secure_web_socket_session.h"
#include "proto/sdp.h"

namespace p4sfu {

    struct ClientMeta {
        bool registered        = false;
        unsigned sessionId     = 0;
        unsigned participantId = 0;

        bool operator==(const ClientMeta& other) const;
        bool operator<(const ClientMeta& other) const;
    };

    class ControllerClientConnection {

    public:
        ControllerClientConnection() = delete;
        ControllerClientConnection(const ControllerClientConnection&) = default;
        ControllerClientConnection& operator=(const ControllerClientConnection&) = default;

        explicit ControllerClientConnection(SecureWebSocketSession<ClientMeta>& s);

        void sendHello();
        void sendOffer(const SDP::SessionDescription& offer, unsigned relatedParticipantId);
        void sendAnswer(const SDP::SessionDescription& answer, unsigned relatedParticipantId);

        inline const asio::ip::tcp::endpoint& remote() const {
            return _ws->remote();
        }

        inline ClientMeta& meta() {
            return _ws->meta();
        }

    private:
        std::shared_ptr<SecureWebSocketSession<ClientMeta>> _ws;
    };
}

#endif
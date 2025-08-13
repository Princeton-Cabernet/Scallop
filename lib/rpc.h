
#ifndef P4SFU_RPC_H
#define P4SFU_RPC_H

#include "net/net.h"
#include "stream.h"
#include <nlohmann/json.hpp>
#include <ostream>

namespace json = nlohmann;

namespace rpc {

    struct Message {
        explicit Message(std::string type);
        [[nodiscard]] virtual std::string to_json() const = 0;

        std::string type;
    };


    namespace sw {

        struct Hello : Message {

            static const std::string TYPE_IDENT;

            Hello();
            explicit Hello(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            std::optional<unsigned> switchId = std::nullopt;
        };

        struct AddParticipant : Message {

            static const std::string TYPE_IDENT;

            AddParticipant();
            explicit AddParticipant(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            unsigned sessionId = 0;
            unsigned participantId = 0;
            net::IPv4 ip = {};
        };

        struct AddStream : Message {

            static const std::string TYPE_IDENT;

            AddStream();
            explicit AddStream(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            unsigned sessionId = 0;
            unsigned participantId = 0;
            net::IPv4 ip;
            std::uint16_t port = 0;
            std::string iceUfrag;
            std::string icePwd;
            p4sfu::SSRC mainSSRC = 0;
            p4sfu::SSRC rtxSSRC = 0;
            p4sfu::SSRC rtcpSSRC = 0;
            p4sfu::SSRC rtcpRtxSSRC = 0;
            p4sfu::MediaType mediaType;
            SDP::Direction direction;
        };

        inline std::ostream& operator<<(std::ostream& os, const AddStream& msg) {
            return os << "AddStream: "
                      << "sessionId=" << msg.sessionId << ", "
                      << "participantId=" << msg.participantId << ", "
                      << "ip/port=" << msg.ip << ":" << msg.port << ", "
                      << "iceUfrag/Pwd=" << msg.iceUfrag << "/" << msg.icePwd << ", "
                      << "ssrc=" << msg.mainSSRC << "/" << msg.rtxSSRC << ", "
                      << "rtcpSSRC=" << msg.rtcpSSRC << "/" << msg.rtcpRtxSSRC << ", "
                      << "mediaType=" << p4sfu::mediaTypeString(msg.mediaType) << ", "
                      << "direction=" << SDP::directionString(msg.direction);
        }
    }

    namespace cl {

        struct Hello : Message {

            static const std::string TYPE_IDENT;

            Hello();
            explicit Hello(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            std::optional<unsigned> sessionId = std::nullopt;
            std::optional<unsigned> participantId = std::nullopt;
        };

        struct Offer : Message {

            static const std::string TYPE_IDENT;

            Offer();
            explicit Offer(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            std::string sdp;
            unsigned participantId = 0;
        };

        struct Answer : Message {

            static const std::string TYPE_IDENT;

            Answer();
            explicit Answer(const json::json& msg);
            [[nodiscard]] std::string to_json() const override;

            std::string sdp;
            unsigned participantId = 0;
        };
    }
}

#endif

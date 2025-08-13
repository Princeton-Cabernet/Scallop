
#ifndef P4_SFU_SIGNALING_MESSAGE_H
#define P4_SFU_SIGNALING_MESSAGE_H

#include "proto/sdp.h"

#include <nlohmann/json.hpp>
namespace json = nlohmann;

namespace p4sfu {

    class SignalingMessage {

    public:
        enum class Type {
            offer  = 0,
            answer = 1
        };

        SignalingMessage() = delete;
        explicit SignalingMessage(Type type, unsigned toParticipant,
            const SDP::SessionDescription& sessionDescription)
            : type(type), toParticipant(toParticipant), sessionDescription(sessionDescription) { }

        SignalingMessage(const SignalingMessage&) = default;
        SignalingMessage& operator=(const SignalingMessage&) = default;

        [[nodiscard]] std::string jsonString() const {

            json::json msg;
            msg["type"] = typeToString(type);
            msg["sdp"] = sessionDescription.escapedString();
            return msg.dump();
        }

        static std::string typeToString(Type t)  {
            switch (t) {
                case Type::offer:  return "offer";
                case Type::answer: return "answer";
            }

            return "";
        }

        Type type;
        unsigned toParticipant;
        SDP::SessionDescription sessionDescription;
    };
}

#endif


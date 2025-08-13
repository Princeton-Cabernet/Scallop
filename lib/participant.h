
#ifndef P4_SFU_PARTICIPANT_H
#define P4_SFU_PARTICIPANT_H

#include <set>
#include <map>
#include <string>
#include "stream.h"

namespace p4sfu {

    class Participant {

        class Transceiver {
        public:
            std::vector<Stream> streams = {};
            //TODO:  iceCandidates should probably be a set:
            std::vector<SDP::ICECandidate> _iceCandidates = {};
            void addStream(const Stream& stream);
            void updateICECandidatesFromSessionDescription(const SDP::SessionDescription& sdp);
            void addICECandidate(const SDP::ICECandidate& c);
            [[nodiscard]] const std::vector<SDP::ICECandidate>& iceCandidates() const;
            [[nodiscard]] bool hasICECandidate(const SDP::ICECandidate& c) const;
            [[nodiscard]] const SDP::ICECandidate& highestPriorityICECandidate() const;
            [[nodiscard]] bool hasStreamWithMid(const std::string& mid) const;
        };

        class Sender : public Transceiver {
        };
        class Receiver : public Transceiver {
        };

    public:
        enum class SignalingState : unsigned {
            stable      = 0, // no pending messages
            //! initiating participant, sent offer, waits for answer after signaling
            //! to remaining participants is complete
            gotOffer    = 1,
            //! not initating participant, offer sent to this participant, wait for answer
            awaitAnswer = 2
        };

        friend class Session;

        [[nodiscard]] unsigned id() const;

        void addOutgoingStream(const Stream& s);
        void addIncomingStream(unsigned sendingParticipant, Stream& s);

        [[nodiscard]] const std::vector<Stream>& outgoingStreams() const;
        [[nodiscard]] const std::vector<Stream> incomingStreams() const;
        [[nodiscard]] bool sendsStreamWithMid(const std::string& mid) const;
        [[nodiscard]] const Stream& sendStreamWithMid(const std::string& mid) const;
        [[nodiscard]] SignalingState signalingState() const;

    private:
        unsigned _id = 0;
        std::string _controlIp;
        unsigned short _controlPort;
        Sender _sender;
        std::map<unsigned, Receiver> _receivers;

        SignalingState _signalingState = SignalingState::stable;
    };
}

static std::ostream& operator<<(std::ostream& os, const p4sfu::Participant::SignalingState& ss) {

    switch (ss) {
        case p4sfu::Participant::SignalingState::stable:      return (os << "stable");
        case p4sfu::Participant::SignalingState::gotOffer:    return (os << "gotOffer");
        case p4sfu::Participant::SignalingState::awaitAnswer: return (os << "awaitAnswer");
    }

    return os;
}

#endif

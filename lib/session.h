
#ifndef P4_SFU_SESSION_H
#define P4_SFU_SESSION_H

#include <map>
#include <set>
#include <queue>
#include <stack>
#include <string>
#include <utility>

#include "net/net.h"
#include "participant.h"
#include "proto/sdp.h"
#include "sfu_config.h"
#include "signaling_message.h"

namespace p4sfu {
    class Session {
    public:

        explicit Session(unsigned id, /*const Config& config,*/ const SFUConfig& sfuConfig);
        Session(const Session&)            = delete;
        Session& operator=(const Session&) = delete;
        Session(Session&&) noexcept        = default;
        Session& operator=(Session&&)      = default;

        #pragma mark - Participant Management

        [[nodiscard]] unsigned id() const;
        [[nodiscard]] bool participantExists(const std::string& ip, unsigned short port) const;
        [[nodiscard]] bool participantExists(unsigned id) const;
        [[nodiscard]] unsigned addParticipant(const std::string& ip, unsigned short port);
        [[nodiscard]] unsigned countParticipants() const;
        [[nodiscard]] p4sfu::Participant& getParticipant(unsigned id);
        [[nodiscard]] p4sfu::Participant& operator[](unsigned id);
        [[nodiscard]] const std::map<unsigned, p4sfu::Participant>& participants() const;
        void removeParticipant(unsigned id);

        #pragma mark - Signaling

        //! returns a vector containing the ids of all participants currently in a given state
        [[nodiscard]] std::vector<unsigned> particpantsInState(
                p4sfu::Participant::SignalingState state) const;

        //! returns the id of the participant in a given state if there is exactly one
        //! @throws std::logic_error if multiple or none are in that state
        [[nodiscard]] unsigned participantInState(p4sfu::Participant::SignalingState state) const;

        //! returns whether there is an active signaling transaction at the moment
        [[nodiscard]] bool signalingActive() const;

        //! adds a received offer to a participant
        void addOffer(unsigned fromParticipantId, const SDP::SessionDescription& offer);

        //! generates an SDP offer incorporating the current state of the session
        [[nodiscard]] SDP::SessionDescription getOffer(unsigned forParticipantId);

        //! adds an answer in response to a previous offer
        void addAnswer(unsigned fromParticipantId, const SDP::SessionDescription& answer);

        //! generates an answer to be sent in response to a previous offer
        [[nodiscard]] SDP::SessionDescription getAnswer(unsigned forParticipantId);

        //! counts the number of unique media streams (i.e., SSRCs) within the session
        [[nodiscard]] unsigned countStreams() const;

        #pragma mark - Callbacks

        void onNewStream(std::function<void
                (const Session&, unsigned participantId,const Stream&)>&& handler);

        #pragma mark - Etc.

        [[nodiscard]] std::string debugSummary() const;

    private:
        // std::vector<Stream>::iterator _getSendStream(const std::string& mid);

        unsigned _id = 0;
        unsigned _nextParticipantId = 0;
        SFUConfig _sfuConfig;
        std::map<unsigned, p4sfu::Participant> _participants;
        std::function<void (const Session&, unsigned participantId, const Stream&)> _onNewStream;
    };
}

#endif

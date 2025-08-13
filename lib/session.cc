
#include "session.h"
#include "stream.h"
#include "participant.h"
#include "net/net.h"

#include <algorithm>

p4sfu::Session::Session(unsigned id, const SFUConfig& sfuConfig)
    : _id(id), _sfuConfig(sfuConfig) { }

unsigned p4sfu::Session::id() const {

    return _id;
}

[[nodiscard]] bool p4sfu::Session::participantExists(const std::string& ip,
                                                     unsigned short port) const {

    auto it = std::find_if(_participants.begin(), _participants.end(), [&ip, &port](auto& p) {
        return p.second._controlIp == ip && p.second._controlPort == port;
    });

    return it != _participants.end();
}

[[nodiscard]] bool p4sfu::Session::participantExists(unsigned id) const {

    return _participants.find(id) != _participants.end();
}

[[nodiscard]] unsigned p4sfu::Session::addParticipant(const std::string& ip, unsigned short port) {

    if (participantExists(ip, port)) {
        throw std::logic_error("Session: addParticipant: participant already exists");
    }

    Participant p;
    p._id = ++_nextParticipantId;
    p._controlIp = ip;
    p._controlPort = {port};

    auto insert_res = _participants.insert({p._id, p});

    if (insert_res.second) {
        return p._id;
    } else {
        throw std::runtime_error("Session: addParticipant: failed adding participant");
    }
}

[[nodiscard]] unsigned p4sfu::Session::countParticipants() const {
    return _participants.size();
}

[[nodiscard]] p4sfu::Participant& p4sfu::Session::getParticipant(unsigned id) {

    auto it = _participants.find(id);

    if (it != _participants.end()) {
        return it->second;
    } else {
        throw std::logic_error("Session: getParticipant: participant does not exist");
    }
}

[[nodiscard]] p4sfu::Participant& p4sfu::Session::operator[](unsigned id) {

        return getParticipant(id);
}

[[nodiscard]] const std::map<unsigned, p4sfu::Participant>& p4sfu::Session::participants() const {

    return _participants;
}

void p4sfu::Session::removeParticipant(unsigned id) {

    auto it = _participants.find(id);

    if (it != _participants.end()) {
        _participants.erase(it);
    } else {
        throw std::logic_error("Session: removeParticipant: participant does not exist");
    }
}

std::vector<unsigned> p4sfu::Session::particpantsInState(
        p4sfu::Participant::SignalingState state) const {

    std::vector<unsigned> participants;

    for (const auto& [_, p]: _participants) {
        if (p.signalingState() == state) {
            participants.push_back(p.id());
        }
    }

    return participants;
}

[[nodiscard]] unsigned p4sfu::Session::participantInState(
        p4sfu::Participant::SignalingState state) const {

    auto participants = particpantsInState(state);

    if (participants.size() == 1) {
        return participants[0];
    } else if (participants.size() > 1) {
        throw std::logic_error("Session: participantInState: multiple participants in state");
    } else {
        throw std::logic_error("Session: participantInState: no participants in state");
    }
}

bool p4sfu::Session::signalingActive() const {

    return std::any_of(_participants.begin(), _participants.end(), [](const auto& p) {
        return p.second.signalingState() != Participant::SignalingState::stable;
    });
}

void p4sfu::Session::addOffer(unsigned fromParticipantId, const SDP::SessionDescription& offer) {

    if (!participantExists(fromParticipantId)) {
        throw std::logic_error("Session: addOffer: participant does not exist");
    }

    if (signalingActive()) {
        throw std::logic_error("Session: addOffer: there is already an active signaling session");
    }

    _participants[fromParticipantId]._sender.updateICECandidatesFromSessionDescription(offer);

    for (const auto& media : offer.mediaDescriptions) {
        if (media.direction && media.direction == SDP::Direction::sendonly) {
            // only add new stream if the mid is not yet added
            if (!_participants[fromParticipantId].sendsStreamWithMid(*media.mid)) {
                auto mediaCopy = media;

                // use previously added highest-priority ice candidate if offer does not carry
                // ice candidates
                if (mediaCopy.iceCandidates.empty()) {
                    if (_participants[fromParticipantId]._sender.iceCandidates().empty()) {
                        throw std::logic_error("Session: addOffer: no ice candidates in offer");
                    } else {
                        mediaCopy.iceCandidates.push_back(
                            _participants[fromParticipantId]._sender.iceCandidates()[0]);
                    }
                }

                p4sfu::Stream stream(mediaCopy, _sfuConfig);
                _participants[fromParticipantId].addOutgoingStream(stream);

                if (_onNewStream) {
                    _onNewStream(*this, fromParticipantId, stream);
                } else {
                    throw std::logic_error("Session: addOffer: no new stream callback set");
                }
            }
        }
    }

    _participants[fromParticipantId]._signalingState = Participant::SignalingState::gotOffer;
}

SDP::SessionDescription p4sfu::Session::getOffer(unsigned forParticipantId) {

    if (!participantExists(forParticipantId)) {
        throw std::logic_error("Session: getOffer: participant does not exist");
    }

    SDP::SessionDescription offer(_sfuConfig.sessionTemplate());

    // set signaling status of participant this offer is for to awaitAnswer
    getParticipant(forParticipantId)._signalingState = Participant::SignalingState::awaitAnswer;

    // get reference to participant who initiated the signaling transaction
    auto initPartId = participantInState(Participant::SignalingState::gotOffer);

    // iterate over the streams that the initiating participant is sending
    for (const auto& s: getParticipant(initPartId).outgoingStreams()) {

        auto media = s.offer(_sfuConfig);
        media.direction = SDP::Direction::sendonly;

        if (s.description().type == SDP::MediaDescription::Type::video) {
            media.extensions = _sfuConfig.videoRTPExtensions();

            // add supported feedback modes to all media formats except rtx
            for (auto& format: media.mediaFormats) {
                if (format.codec != "rtx") {
                    format.fbTypes = _sfuConfig.videoFbTypes();
                }
            }

        } else if (s.description().type == SDP::MediaDescription::Type::audio) {
            media.extensions = _sfuConfig.audioRTPExtensions();

            // audio does not use rtx, so no need to check
            for (auto& format: media.mediaFormats) {
                format.fbTypes = _sfuConfig.audioFbTypes();
            }
        }

        // add media description to offer
        offer.mediaDescriptions.push_back(media);
    }

    return offer;
}

void p4sfu::Session::addAnswer(unsigned fromParticipantId, const SDP::SessionDescription& answer) {

    auto& offerFrom = _participants[participantInState(Participant::SignalingState::gotOffer)];
    auto& answerFrom = _participants[fromParticipantId];

    if (answerFrom._signalingState != Participant::SignalingState::awaitAnswer) {
        throw std::logic_error("Session: addAnswer: unexpected answer");
    }

    for (auto& media: answer.mediaDescriptions) {

        for (auto const& iceCandidate: media.iceCandidates) {
            answerFrom._receivers[offerFrom._id].addICECandidate(iceCandidate);
        }
    }

    for (auto& media: answer.mediaDescriptions) {

        if (!media.mid) {
            throw std::logic_error("Session: addAnswer: answer media desc. does not have mid");
        }

        const auto& sendStream = offerFrom.sendStreamWithMid(*media.mid);
        assert(sendStream.description().ssrcDescription.has_value());

        auto mediaCopy = media;

        // use previously added highest-priority ice candidate if offer does not carry
        // ice candidates
        if (mediaCopy.iceCandidates.empty()) {
            if (answerFrom._receivers[offerFrom._id].iceCandidates().empty()) {
                throw std::logic_error("Session: addOffer: no ice candidates in offer");
            } else {
                mediaCopy.iceCandidates.push_back(
                        answerFrom._receivers[offerFrom._id].iceCandidates()[0]);
            }
        }

        p4sfu::Stream stream{mediaCopy, _sfuConfig, *(sendStream.description().ssrcDescription)};

        // only add the stream when it does not yet exist (based on mid)
        //  - streams that are part of the session description can already be present in the
        //    forwarding configuration from a previvous signaling transaction
        if (!answerFrom._receivers[offerFrom._id].hasStreamWithMid(*(media.mid))) {

            answerFrom._receivers[offerFrom._id].addStream(stream);

            if (_onNewStream) {
                _onNewStream(*this, fromParticipantId, stream);
            } else {
                throw std::logic_error("Session: addOffer: no new stream callback set");
            }
        }
    }

    answerFrom._signalingState = Participant::SignalingState::stable;
}

SDP::SessionDescription p4sfu::Session::getAnswer(unsigned forParticipantId) {

    if (!participantExists(forParticipantId)) {
        throw std::logic_error("Session: getAnswer: participant does not exist");
    }

    if (_participants[forParticipantId]._signalingState != Participant::SignalingState::gotOffer) {
        throw std::logic_error("Session: getAnswer: no previous offer");
    }

    SDP::SessionDescription answer{_sfuConfig.sessionTemplate()};
    answer.origin.sessionId = _id;

    auto& participant = getParticipant(forParticipantId);

    for (const auto& s: participant.outgoingStreams()) {

        auto media = s.offer(_sfuConfig);

        media.direction = SDP::Direction::recvonly;

        // omit some information for streams that originate at participant answer is for
        media.ssrcDescription = std::nullopt;
        media.msid = std::nullopt;

        if (s.description().type == SDP::MediaDescription::Type::video) {
            media.extensions = _sfuConfig.videoRTPExtensions();

            // add supported feedback modes to all media formats except rtx
            for (auto& format: media.mediaFormats) {
                if (format.codec != "rtx") {
                    format.fbTypes = _sfuConfig.videoFbTypes();
                }
            }

        } else if (s.description().type == SDP::MediaDescription::Type::audio) {
            media.extensions = _sfuConfig.audioRTPExtensions();

            // audio does not use rtx, so no need to check
            for (auto& format: media.mediaFormats) {
                format.fbTypes = _sfuConfig.audioFbTypes();
            }
        }

        answer.mediaDescriptions.push_back(media);
    }

    participant._signalingState = Participant::SignalingState::stable;

    return answer;
}

unsigned p4sfu::Session::countStreams() const {

    return std::accumulate(_participants.begin(), _participants.end(), 0,
        [](unsigned sum, const auto& p) {
        return sum + p.second.outgoingStreams().size();
    });
}

void p4sfu::Session::onNewStream(std::function<void
    (const Session&, unsigned participantId, const Stream&)>&& handler) {

    _onNewStream = std::move(handler);
}

[[nodiscard]] std::string p4sfu::Session::debugSummary() const {

    std::stringstream ss;

    ss << "Session: id=" << _id << ", #participants=" << _participants.size() << ", signalingActive="
       << (signalingActive() ? "true"  : "false") << std::endl;

    for (const auto& [_, p]: _participants) {
        ss << " - participant: id=" << p.id()
           << ", outgoingStreams=" << p.outgoingStreams().size()
           << ", incomingStreams=" << p.incomingStreams().size()
           << ", signalingState=" << p.signalingState() << std::endl;

        for (const auto& s : p.outgoingStreams()) {

            ss << "    - tx-stream: type="
               << (s.description().type == SDP::MediaDescription::Type::audio ? "audio" : "video")
               << ", mid=" << s.description().mid.value_or("")
               << ", msid=" << s.description().msid->appData;

            if (s.description().ssrcDescription->ssrcs.size() == 1) {
                ss << ", ssrc=" << s.description().ssrcDescription->ssrcs[0].ssrc;
            } else if (s.description().ssrcDescription->ssrcs.size() == 2) {
                ss << ", ssrc=" << s.description().ssrcDescription->ssrcs[0].ssrc << "/"
                   << s.description().ssrcDescription->ssrcs[1].ssrc;
            }

            ss << std::endl;
        }

        for (const auto& s : p.incomingStreams()) {
            ss << "    - rx-stream: type="
               << (s.description().type == SDP::MediaDescription::Type::audio ? "audio" : "video")
               << ", mid=" << s.description().mid.value_or("") << std::endl;
        }
    }

    return ss.str();
}

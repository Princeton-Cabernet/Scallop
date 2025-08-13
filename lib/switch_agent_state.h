#ifndef P4SFU_SWITCH_AGENT_STATE_H
#define P4SFU_SWITCH_AGENT_STATE_H

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "p4sfu.h"
#include "net/net.h"
#include "av1.h"

namespace p4sfu {

    class SwitchAgentState {

    public:

        struct Stream {
            unsigned sessionId          = 0;
            unsigned sendingParticipant = 0;
            net::IPv4Port addr          = {};
            SSRC ssrc                   = 0;
            MediaType type              = MediaType::audio;
            bool rtx                    = false;
        };

        struct SendStream : Stream {
            std::vector<unsigned> receiveStreamIds = {};
        };

        struct ReceiveStream : Stream {
            unsigned sendStreamId                     = 0;
            unsigned receivingParticipant             = 0;
            av1::svc::L1T3::DecodeTarget decodeTarget = av1::svc::L1T3::DecodeTarget::hi;
            std::vector<unsigned> bandwidthEstimates  = {};
        };


        using SendStreamIterator = std::unordered_map<unsigned, SendStream>::iterator;
        using ReceiveStreamIterator = std::unordered_map<unsigned, ReceiveStream>::iterator;

        void addSendStream(unsigned sessionId, unsigned participantId, const net::IPv4Port& addr,
                           MediaType type, SSRC mainSSRC, SSRC rtxSSRC = 0) {

            // add session if it does not exist

            auto sessionIt = _sessions.find(sessionId);

            if (sessionIt == _sessions.end()) {
                sessionIt = _sessions.insert({
                     sessionId, std::unordered_map<SSRC, unsigned>()
                }).first;
            }

            if (sessionIt->second.find(mainSSRC) != sessionIt->second.end()) {
                throw std::logic_error("SwitchAgentState: send stream already exists: ssrc="
                                       + std::to_string(mainSSRC));
            }

            if (sessionIt->second.find(rtxSSRC) != sessionIt->second.end()) {
                throw std::logic_error("SwitchAgentState: send stream already exists: ssrc="
                                       + std::to_string(mainSSRC));
            }

            auto mainStreamId = _next.sendStreamId++;

            SendStream s1;
            s1.sessionId               = sessionId;
            s1.sendingParticipant      = participantId;
            s1.addr                    = addr;
            s1.ssrc                    = mainSSRC;
            s1.type                    = type;
            _sendStreams[mainStreamId] = s1;

            sessionIt->second.insert({mainSSRC, mainStreamId});

            if (rtxSSRC != 0) {

                auto rtxStreamId = _next.sendStreamId++;

                SendStream s2;
                s2.sessionId              = sessionId;
                s2.sendingParticipant     = participantId;
                s2.addr                   = addr;
                s2.ssrc                   = rtxSSRC;
                s2.type                   = type;
                s2.rtx                    = true;
                _sendStreams[rtxStreamId] = s2;

                sessionIt->second.insert({rtxSSRC, rtxStreamId});
            }
        }

        void addReceiveStream(unsigned sessionId, unsigned fromParticipantId,
                              unsigned toParticipantId, const net::IPv4Port& addr,
                              SSRC mainSSRC, SSRC rtxSSRC = 0) {

            auto mainSendStreamIt = getSendStream(sessionId, mainSSRC);

            if (mainSendStreamIt == _sendStreams.end()) {
                throw std::logic_error("SwitchAgentTest: addReceiveStream: no corresponding "
                                       "receive  stream: ssrc=" + std::to_string(mainSSRC));
            }

            auto mainStreamId = _next.receiveStreamId++;

            ReceiveStream r1;
            r1.sessionId                  = sessionId;
            r1.sendingParticipant         = fromParticipantId;
            r1.addr                       = addr;
            r1.ssrc                       = mainSSRC;
            r1.receivingParticipant       = toParticipantId;
            r1.type                       = mainSendStreamIt->second.type;
            r1.sendStreamId               = mainSendStreamIt->first;
            r1.sendingParticipant         = mainSendStreamIt->second.sendingParticipant;
            _receiveStreams[mainStreamId] = r1;

            mainSendStreamIt->second.receiveStreamIds.push_back(mainStreamId);

            if (rtxSSRC != 0) {

                auto rtxSendStreamIt = getSendStream(sessionId, rtxSSRC);

                if (rtxSendStreamIt == _sendStreams.end()) {
                    throw std::logic_error("SwitchAgentTest: addReceiveStream: no corresponding "
                                           "receive  stream: ssrc=" + std::to_string(mainSSRC));
                }

                auto rtxStreamId = _next.receiveStreamId++;

                ReceiveStream r2;
                r2.sessionId                 = sessionId;
                r2.sendingParticipant        = fromParticipantId;
                r2.addr                      = addr;
                r2.ssrc                      = rtxSSRC;
                r2.receivingParticipant      = toParticipantId;
                r2.type                      = rtxSendStreamIt->second.type;
                r2.rtx                       = true;
                r2.sendStreamId              = rtxSendStreamIt->first;
                r2.sendingParticipant        = rtxSendStreamIt->second.sendingParticipant;
                _receiveStreams[rtxStreamId] = r2;

                rtxSendStreamIt->second.receiveStreamIds.push_back(rtxStreamId);
            }
        }

        [[nodiscard]] const std::unordered_map<unsigned, SendStream>& sendStreams() const {

            return _sendStreams;
        }

        [[nodiscard]] SendStreamIterator getSendStream(unsigned sessionId, SSRC ssrc) {

            return std::ranges::find_if(_sendStreams,
            [&](const std::pair<unsigned, SendStream>& s) {
                return std::tie(s.second.sessionId, s.second.ssrc) == std::tie(sessionId, ssrc);
            });
        }

        [[nodiscard]] const std::unordered_map<unsigned, ReceiveStream>& receiveStreams() const {

            return _receiveStreams;
        }

        [[nodiscard]] ReceiveStreamIterator getReceiveStream(unsigned sessionId, SSRC ssrc,
                                                             unsigned receivingParticipantId) {

            return std::ranges::find_if(_receiveStreams,
                [&](const std::pair<unsigned, ReceiveStream>& s) {
                    return std::tie(s.second.sessionId, s.second.ssrc, s.second.receivingParticipant)
                           == std::tie(sessionId, ssrc, receivingParticipantId);
            });
        }

        [[nodiscard]] ReceiveStreamIterator getReceiveStream(const net::IPv4Port& from, SSRC ssrc) {

            return std::ranges::find_if(_receiveStreams,
            [&](const std::pair<unsigned, ReceiveStream>& s) {
                return std::tie(s.second.addr, s.second.ssrc) == std::tie(from, ssrc);
           });
        }

    private:
        // receiveStreamId -> SendStream
        std::unordered_map<unsigned, SendStream> _sendStreams;

        // sendStreamId -> SendStream
        std::unordered_map<unsigned, ReceiveStream> _receiveStreams;

        // sessionId -> [ ssrc -> sendStreamId ]
        std::unordered_map<unsigned, std::unordered_map<SSRC, unsigned>> _sessions;

        struct { unsigned sendStreamId = 0, receiveStreamId = 0; } _next;
    };
}

#endif

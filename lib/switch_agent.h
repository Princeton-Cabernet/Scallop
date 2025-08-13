
#ifndef P4_SFU_SWITCH_AGENT_H
#define P4_SFU_SWITCH_AGENT_H

#include <boost/asio.hpp>

#include "av1.h"
#include "data_plane_model.h"
#include "log.h"
#include "net/tcp_client.h"
#include "net/udp_server.h"
#include "proto/rtcp.h"
#include "proto/rtp.h"
#include "proto/stun.h"
#include "rpc.h"
#include "stun_agent.h"
#include "switch_agent_state.h"
#include "switch_api.h"
#include "switch_controller_client.h"
#include "switch_statistics.h"
#include "timer.h"

using namespace boost;

namespace p4sfu {

    template <typename DataPlaneType>
    class SwitchAgent {
    public:

        struct Config {
            enum class Type { model, tofino } type = Type::model;
            unsigned      switchId                 = 0;
            std::uint16_t sfuListenPort            = 0;
            std::uint16_t apiListenPort            = 6790;
            std::string   controllerIPv4;
            std::uint16_t controllerPort           = 0;
            std::string   dataPlaneIface; // Tofino only
            std::string   iceUfrag;
            std::string   icePwd;
            unsigned      av1RtpExtId               = 0;
            double        rtpDropRate               = 0;
            bool          verbose                   = false;
        };

        SwitchAgent(const Config& c, DataPlane::Config& dpc)
            : _config(c),
              _io(),
              _controllerClient(_io),
              _api(_io, _config.apiListenPort),
              _dataPlane(std::make_shared<DataPlaneType>(&_io, &dpc)),
              _stunAgent(STUNAgent::Config{stun::Credentials{_config.iceUfrag, _config.icePwd}}),
              _timer(_io, 10 * 1000 /* = 10 seconds */) {

            Log::config = { .level = _config.verbose ? Log::DEBUG : Log::INFO,
                            .printLabel = true };

            if (_config.type == Config::Type::tofino) {

                Log(Log::INFO) << "SwitchAgent: (): running as tofino agent" << std::endl;
                Log(Log::INFO) << "SwitchAgent: (): api-listen-port=" << c.apiListenPort
                               << ", controller=" << c.controllerIPv4 << ":" << c.controllerPort
                               << ", ice-ufrag=" << c.iceUfrag
                               << ", ice-pwd=" << c.icePwd << std::endl;

            } else if(_config.type == Config::Type::model) {

                Log(Log::INFO) << "SwitchAgent: (): running as model" << std::endl;
                Log(Log::INFO) << "SwitchAgent: (): sfu-listen-port=" << c.sfuListenPort
                               << ", api-listen-port=" << c.apiListenPort
                               << ", controller=" << c.controllerIPv4 << ":" << c.controllerPort
                               << ", ice-ufrag=" << c.iceUfrag
                               << ", ice-pwd=" << c.icePwd
                               << ", av1-rtp-ext-id=" << c.av1RtpExtId
                               << ", rtp-drop-rate=" << c.rtpDropRate << std::endl;
            }

            // set up controller client callbacks:

            _controllerClient.onOpen([this](SwitchControllerClient& c) {
                this->_onControllerOpen(c);
            });

            _controllerClient.onHello([this](SwitchControllerClient& c, const rpc::sw::Hello& m) {
                this->_onControllerHello(c, m);
            });

            _controllerClient.onAddParticipant([this](SwitchControllerClient& c,
                                                      const rpc::sw::AddParticipant& m) {
                this->_onAddParticipant(c, m);
            });

            _controllerClient.onAddStream([this](SwitchControllerClient& c,
                    const rpc::sw::AddStream& m) {
                this->_onAddStream(c, m);
            });

            _controllerClient.onClose([this](SwitchControllerClient& c) {
                this->_onControllerClose(c);
            });

            // set up data-plane callback:

            _dataPlane->onPacketToController([this](DataPlane& d, DataPlane::PktIn pkt) {
                this->_onPacketFromDataPlane(d, pkt);
            });

            // set up api callbacks:

            _api.onOpen = [this](SwitchAPI::Connection& c) {
                this->_onAPIOpen(c);
            };

            _api.onGetStreams = [this](SwitchAPIMeta& m) {
                return this->_onAPIGetStreams(m);
            };

            _api.onSetDecodeTarget = [this](SwitchAPIMeta& m, unsigned sessionId, unsigned ssrc,
                                            unsigned participantId, unsigned decodeTarget) {

                return this->_onAPISetDecodeTarget(m, sessionId, ssrc, participantId, decodeTarget);
            };

            _api.onClose = [this](const SwitchAPIMeta& m) {
                this->_onAPIClose(m);
            };

            // set up timer callback:

            _timer.onTimer([this](Timer& t) {
                this->_onTimer(t);
            });

            try {
                // connect to controller:
                _controllerClient.connect(c.controllerIPv4, c.controllerPort);
            } catch (boost::system::system_error& e) {
                Log(Log::ERROR) << "SwitchAgent: (): failed connecting to controller at "
                                << c.controllerIPv4 << ":" << c.controllerPort << ": "
                                << e.what() << std::endl;

                throw std::runtime_error("SwitchAgent: (): failed connecting to controller");
            }
        }

        SwitchAgent(const SwitchAgent&) = delete;
        SwitchAgent& operator=(const SwitchAgent&) = delete;

        int operator()() {
            try {
                _io.run();
                return 0;
            } catch (std::exception& e) {
                Log(Log::ERROR) << "SwitchAgent: SwitchAgent: io_context::run failed: " << e.what()
                                << std::endl;
                return -1;
            }
        }

    private:

        Config _config;
        asio::io_context _io;
        SwitchControllerClient _controllerClient;
        SwitchAPI _api;
        std::shared_ptr<DataPlane> _dataPlane;
        STUNAgent _stunAgent;
        Timer _timer;

        SwitchAgentState _state;

        #pragma mark operations:

        bool _adjustDecodeTarget(unsigned sessionId, SSRC ssrc, unsigned participantId,
                                 unsigned decodeTarget) {

            if (decodeTarget > av1::svc::L1T3::MAX_IDENT) {
                Log(Log::ERROR) << "SwitchAgent: _adjustDecodeTarget: invalid decode target: "
                                << decodeTarget << std::endl;
                return false;
            }

            auto rs = _state.getReceiveStream(sessionId, ssrc, participantId);

            if (rs == _state.receiveStreams().end()) {
                Log(Log::ERROR) << "SwitchAgent: _adjustDecodeTarget: receive stream not found: "
                                << "sessionId=" << sessionId << ", ssrc=" << ssrc
                                << ", participantId=" << participantId << std::endl;
                return false;
            }

            auto ss = _state.getSendStream(sessionId, rs->second.ssrc);

            if (ss == _state.sendStreams().end()) {
                Log(Log::ERROR) << "SwitchAgent: _adjustDecodeTarget: send stream not found"
                                << std::endl;
                return false;
            }

            if (rs->second.type != MediaType::video || rs->second.rtx) {

                Log(Log::ERROR) << "SwitchAgent: _adjustDecodeTarget: "
                                << "decode target only supported for non-rtx video streams"
                                << std::endl;
                return false;
            }

            _dataPlane->adjustDecodeTarget(ss->second.addr, rs->second.addr, ss->second.ssrc,
                                           decodeTarget);

            rs->second.decodeTarget = av1::svc::L1T3::decodeTargetFromNumIdentifier(decodeTarget);

            return true;
        }

        #pragma mark controller-side events:

        void _onControllerOpen(SwitchControllerClient& c) {

            Log(Log::INFO) << "SwitchAgent: _onControllerOpen: connected to controller" << std::endl;
            c.sendHello(_config.switchId);
        }

        void _onControllerHello(SwitchControllerClient& c, const rpc::sw::Hello& m) {

            Log(Log::INFO) << "SwitchAgent: _onControllerHello: received hello from controller"
                           << std::endl;
        }

        void _onAddParticipant(SwitchControllerClient& c, const rpc::sw::AddParticipant& m) {

            Log(Log::INFO) << "SwitchAgent: _onAddParticipant: new participant: sessionId="
                           << m.sessionId << ", participantId=" << m.participantId << ", ip="
                           << m.ip << std::endl;
        }

        void _onAddStream(SwitchControllerClient& c, const rpc::sw::AddStream& m) {

            Log(Log::INFO) << "SwitchAgent: _onAddStream: " << m << std::endl;

            // add entry for send stream with address
            if (m.direction == SDP::Direction::sendonly) {

                // _sendStreams[m.sessionId][m.mainSSRC] = net::IPv4Port{m.ip, m.port};

                _state.addSendStream(m.sessionId, m.participantId, net::IPv4Port{m.ip, m.port},
                                     m.mediaType, m.mainSSRC, m.rtxSSRC);
            }

            if (!_stunAgent.hasPeer(net::IPv4Port{m.ip, m.port})) {

                _stunAgent.addPeer(net::IPv4Port{m.ip, m.port},
                                   stun::Credentials{m.iceUfrag,m.icePwd});
                Log(Log::INFO) << "SwitchAgent: _onAddStream: added STUN peer: "
                               << "ip=" << m.ip << ", port=" << m.port << std::endl;

            } else {

                Log(Log::INFO) << "SwitchAgent: _onAddStream: STUN peer already exists: "
                               << "ip=" << m.ip << ", port=" << m.port << std::endl;
            }

            // receive streams are additionally set up in the data plane to enable forwarding:
            if (m.direction == SDP::Direction::recvonly) {

                auto sendStreamIt = _state.getSendStream(m.sessionId, m.mainSSRC);

                if (sendStreamIt == _state.sendStreams().end()) {
                    Log(Log::ERROR) << "SwitchAgent: _onAddStream: no sender address found for "
                                    << "sessionId=" << m.sessionId << ", mainSSRC=" << m.mainSSRC
                                    << std::endl;
                    return;
                }

                const auto& sendStream = sendStreamIt->second;

                /*
                auto senderAddrIt = _sendStreams[m.sessionId].find(m.mainSSRC);

                if (senderAddrIt == _sendStreams[m.sessionId].end()) {
                    Log(Log::ERROR) << "SwitchAgent: _onAddStream: no sender address found for "
                                    << "sessionId=" << m.sessionId << ", mainSSRC=" << m.mainSSRC
                                    << std::endl;
                    return;
                }
                */

                _state.addReceiveStream(m.sessionId, sendStream.sendingParticipant, m.participantId,
                                        net::IPv4Port{m.ip, m.port}, m.mainSSRC, m.rtxSSRC);


                Log(Log::INFO) << "SwitchAgent: _onAddStream: add receive stream" << std::endl;
                _dataPlane->addStream(DataPlane::Stream{
                    .src         = sendStream.addr,
                    .dst         = net::IPv4Port{m.ip, m.port},
                    .ssrc        = m.mainSSRC,
                    .rtxSsrc     = m.rtxSSRC,
                    .rtcpSsrc    = m.rtcpSSRC,
                    .rtcpRtxSsrc = m.rtcpRtxSSRC,
                    .egressPort  = 0
                });

            } else if (m.direction == SDP::Direction::sendonly) {

                if (_config.type == Config::Type::model) {

                    Log(Log::INFO) << "SwitchAgent: _onAddStream: add send stream" << std::endl;

                    _dataPlane->addStream(DataPlane::Stream{
                        .src         = net::IPv4Port{0, 0},
                        .dst         = net::IPv4Port{m.ip, m.port},
                        .ssrc        = m.mainSSRC,
                        .rtxSsrc     = m.rtxSSRC,
                        .rtcpSsrc    = 0,
                        .rtcpRtxSsrc = 0,
                        .egressPort  = 0
                    });

                } else if (_config.type == Config::Type::tofino) {
                    Log(Log::INFO) << "SwitchAgent: _onAddStream: do not add send stream (Tofino)"
                                   << std::endl;
                }
            }
        }

        void _onControllerClose(SwitchControllerClient& c) {

            Log(Log::INFO) << "SwitchAgent: _onControllerClose: close from controller" << std::endl;
        }

        #pragma mark data-plane side events:

        void _onPacketFromDataPlane(DataPlane& dataPlane, DataPlane::PktIn& pkt) {

            Log(Log::DEBUG) << "SwitchAgent: _onPacketFromDataPlane: len=" << pkt.len << std::endl;

            switch (pkt.reason) {
                case DataPlane::PktIn::Reason::stun: _handleSTUN(dataPlane, pkt); break;
                case DataPlane::PktIn::Reason::rtcp: _handleRTCP(dataPlane, pkt); break;
                case DataPlane::PktIn::Reason::av1:  _handleAV1(dataPlane, pkt);  break;
            }
        }

        #pragma mark packet handlers:

        void _handleSTUN(DataPlane& dataPlane, DataPlane::PktIn& pkt) {

            Log(Log::DEBUG) << "SwitchAgent: _handleSTUN: len=" << pkt.len << std::endl;

            if (stun::bufferContainsSTUNBindingRequest(pkt.buf, pkt.len)) {
                _handleSTUNBindingRequest(dataPlane, pkt);
            } else {
                Log(Log::ERROR) << "SwitchAgent: _handleSTUN: unknown STUN message, ignore."
                                << std::endl;
            }
        }

        void _handleSTUNBindingRequest(DataPlane& dataPlane, DataPlane::PktIn& pkt) {

            Log(Log::DEBUG) << "SwitchAgent: _handleSTUNBindingRequest: len=" << pkt.len
                            << std::endl;

            auto stunResult = _stunAgent.validate(pkt.from, pkt.buf, pkt.len);

            if (stunResult.success) {

                if (stunResult.initial) {
                    Log(Log::INFO) << "SwitchAgent: initial STUN validation succeeded for "
                                   << pkt.from << std::endl;
                } else {
                    Log(Log::DEBUG) << "SwitchAgent: STUN validation succeeded" << std::endl;
                }

                _dataPlane->sendPacket(DataPlane::PktOut{
                    pkt.from, _stunAgent.msgBuf(), _stunAgent.msgLen()
                });

                Log(Log::DEBUG) << "SwitchAgent: sent STUN response to " << pkt.from << std::endl;

            } else {

                Log(Log::ERROR) << "SwitchAgent: STUN validation failed: from=" << pkt.from
                                << ", error=" << stunResult.message << std::endl;
            }
        }

        void _handleAV1(DataPlane& dataPlane, DataPlane::PktIn& pkt) {

            const auto* rtp = reinterpret_cast<const rtp::hdr*>(pkt.buf);
            const auto* av1Ptr = rtp->extension_ptr(_config.av1RtpExtId);
            av1::DependencyDescriptor av1;

            if (av1Ptr == nullptr) {
                Log(Log::ERROR) << "SwitchAgent: _handleAV1: no AV1 extension found" << std::endl;
            }

            if (rtp->extension_profile() == rtp::ext_profile::one_byte) {
                const auto* ext = reinterpret_cast<const rtp::one_byte_extension_hdr*>(av1Ptr);
                av1 = av1::DependencyDescriptor{av1Ptr + 1, ext->len()};
            } else if (rtp->extension_profile() == rtp::ext_profile::two_byte) {
                const auto* ext = reinterpret_cast<const rtp::two_byte_extension_hdr*>(av1Ptr);
                av1 = av1::DependencyDescriptor{av1Ptr + 2, ext->len()};
            } else {
                Log(Log::ERROR) << "SwitchAgent: _handleAV1: unknown extension profile" << std::endl;
                return;
            }

            Log(Log::INFO) << "SwitchAgent: _handleAV1: av1_dd: "
                            << (av1.mandatoryFields().startOfFrame() ? "start, " : "")
                            << (av1.mandatoryFields().endOfFrame() ? "end, " : "")
                            << "tpl_id=" << av1.mandatoryFields().templateId()
                            << ", frame_num=" << av1.mandatoryFields().frameNumber()
                            << std::endl;

            std::stringstream ss;

            for (const auto& [id, tpl]: av1.templates()) {
                ss << " - id=" << id << ", spatial_layer_id=" << tpl.spatial_layer_id
                   << ", temporal_layer_id=" << tpl.temporal_layer_id << ", dtis=[ ";
                for (const auto& dti: tpl.dtis) {
                    ss << av1::dtiString[dti] << " ";
                }
                ss << "]" << std::endl;
            }

            Log(Log::DEBUG) << "SwitchAgent: _handleAV1: av1 ext. desc.: " << std::endl << ss.str();
        }

        void _handleRTCP(DataPlane& dataPlane, DataPlane::PktIn& pkt) {

            for (auto bytesRem = pkt.len; bytesRem > 0;) {

                const auto* rtcp = reinterpret_cast<const rtcp::hdr*>(pkt.buf + pkt.len - bytesRem);

                switch (static_cast<rtcp::pt>(rtcp->pt)) {
                    case rtcp::pt::rr:
                        _processReceiverReport(pkt.from, rtcp);
                        break;
                    case rtcp::pt::psfb:

                        switch (rtcp->fb_fmt()) {
                            case 1:
                                _processPictureLossIndication(pkt.from, rtcp);
                                break;
                            case 15:
                                _processReceiverEstimatedBitrate(pkt.from, rtcp);
                                break;
                            default:
                                Log(Log::WARN) << "SwitchAgent: _handleRTCP: unsupported psfb type "
                                               << rtcp->fb_fmt() << std::endl;
                                break;
                        }

                        break;
                    case rtcp::pt::rtpfb:

                        switch (rtcp->fb_fmt()) {
                            case 1:
                                _processNegativeAcknowledgement(pkt.from, rtcp);
                                break;
                            default:
                                Log(Log::WARN) << "SwitchAgent: _handleRTCP: unsupported rtpfb "
                                               << "type " << rtcp->fb_fmt() << std::endl;
                                break;
                        }
                        break;

                    default:
                        Log(Log::WARN) << "SwitchAgent: _handleRTCP: unsupported msg." << std::endl;
                        break;
                }

                bytesRem -= rtcp->byte_len();
            }
        }

        void _processReceiverReport(const net::IPv4Port& from, const rtcp::hdr* rtcp) {

            for (unsigned i = 0; i < rtcp->recep_rep_count(); i++) {
                Log(Log::DEBUG) << "SwitchAgent: _processReceiverReport: "
                                << "from= " << from << ", "
                                << "sender_ssrc=" << std::dec << ntohl(rtcp->sender_ssrc) << ", "
                                << "ssrc=" << std::dec << ntohl(rtcp->data.rr[i].ssrc) << ", "
                                << "frac_lost=" << rtcp->data.rr[i].frac_lost() << ", "
                                << "jitter=" << ntohl(rtcp->data.rr[i].jitter) << std::endl;
            }
        }

        static av1::svc::L1T3::DecodeTarget _decideDecodeTarget(
            const SwitchAgentState::ReceiveStream& stream, const unsigned bitrate) {

            /*
            auto currentTarget = stream.decodeTarget;
            auto& history = stream.bandwidthEstimates;
            */

            if (bitrate / 1e3 > 2000) { // > 2 Mbps
                return av1::svc::L1T3::DecodeTarget::hi;
            } else if (bitrate / 1e3 > 1000) { // > 1 Mbps
                return av1::svc::L1T3::DecodeTarget::mid;
            } else { // < 1 Mbps
                return av1::svc::L1T3::DecodeTarget::lo;
            }
        }

        void _processReceiverEstimatedBitrate(const net::IPv4Port& from, const rtcp::hdr* rtcp) {

            for (auto i = 0; i < rtcp->data.remb.num_ssrcs(); i++) {

                Log(Log::DEBUG) << "SwitchAgent: _processReceiverEstimatedBitrate: "
                                << "from= " << from << ", "
                                << "ssrc=" << std::dec << ntohl(rtcp->data.remb.ssrcs[i]) << ", "
                                << "bit_rate=" << rtcp->data.remb.bit_rate() << std::endl;


                const auto rsIt = _state.getReceiveStream(from, ntohl(rtcp->data.remb.ssrcs[i]));

                if (rsIt == _state.receiveStreams().end()) {

                    Log(Log::ERROR) << "SwitchAgent: _processReceiverEstimatedBitrate: "
                                    << "receive stream not found: ssrc=" << std::dec
                                    << ntohl(rtcp->data.remb.ssrcs[i]) << std::endl;
                    return;
                }

                auto& receiveStream = rsIt->second;
                receiveStream.bandwidthEstimates.push_back(rtcp->data.remb.bit_rate());

                auto newTarget = _decideDecodeTarget(receiveStream, rtcp->data.remb.bit_rate());

                if (newTarget != receiveStream.decodeTarget) {
                    Log(Log::INFO) << "SwitchAgent: _processReceiverEstimatedBitrate: "
                                   << "changing decode target: ssrc=" << std::dec
                                   << ntohl(rtcp->data.remb.ssrcs[i]) << ", "
                                   << "bit_rate=" << rtcp->data.remb.bit_rate() << ", "
                                   << "new_target=" << static_cast<unsigned>(newTarget)
                                   << std::endl;

                    // update state:
                    receiveStream.decodeTarget = newTarget;

                    // change decode target in data plane:
                    _adjustDecodeTarget(receiveStream.sessionId, receiveStream.ssrc,
                                        receiveStream.receivingParticipant,
                                        static_cast<unsigned>(newTarget));
                }
            }
        }

        void _processPictureLossIndication(const net::IPv4Port& from, const rtcp::hdr* rtcp) {

            Log(Log::INFO) << "SwitchAgent: _processPictureLossIndication: "
                           << "from= " << from << ", "
                           << "ssrc=" << std::dec << ntohl(rtcp->data.pli.ssrc) << std::endl;
        }

        void _processNegativeAcknowledgement(const net::IPv4Port& from, const rtcp::hdr* rtcp) {

            Log(Log::INFO) << "SwitchAgent: _processNegativeAcknowledgement: "
                           << "from= " << from << ", "
                           << "ssrc=" << std::dec << ntohl(rtcp->data.nack.ssrc) << std::endl;
        }

        #pragma mark api events:

        void _onAPIOpen(SwitchAPI::Connection& c) {
            Log(Log::INFO) << "SwitchAgent: _onAPIOpen: new connection" << std::endl;
        }


        json::json _onAPIGetStreams(SwitchAPIMeta& m) {

            Log(Log::INFO) << "SwitchAgent: _onAPIGetStreams: get streams" << std::endl;

            json::json j;
            j["type"] = "streams";
            j["data"]["streams"] = json::json::array();

            for (const auto& [_, stream]: _state.sendStreams()) {

                auto sendStreamJson = json::json::object({
                     { "session_id",  stream.sessionId },
                     { "participant_id", stream.sendingParticipant },
                     { "ssrc",        stream.ssrc },
                     { "ip",   stream.addr.ip().str() },
                     { "port", stream.addr.port() },
                     { "receivers", json::json::array() },
                     { "type", stream.type == MediaType::audio ? "audio" : "video" },
                     { "rtx", stream.rtx }
                });

                for (auto receiveStreamId: stream.receiveStreamIds) {

                    const auto& receiveStreamIt = _state.receiveStreams().find(receiveStreamId);

                    if (receiveStreamIt != _state.receiveStreams().end()) {
                        auto receiveStream = receiveStreamIt->second;

                        auto receiveStreamJson = json::json::object({
                            { "participant_id", receiveStream.receivingParticipant },
                            { "ip", receiveStream.addr.ip().str() },
                            { "port", receiveStream.addr.port() }
                        });

                        if (stream.type == MediaType::video && !stream.rtx) {
                            receiveStreamJson["decode_target"]
                                = (unsigned) receiveStream.decodeTarget;
                        }

                        sendStreamJson["receivers"].push_back(receiveStreamJson);

                    } else {
                        Log(Log::WARN) << "SwitchAgent: _onAPIGetStreams: "
                                       << "receive stream not found: id=" << receiveStreamId
                                       << std::endl;
                    }
                }

                j["data"]["streams"].push_back(sendStreamJson);
            }

            return j;
        }

        json::json _onAPISetDecodeTarget(const SwitchAPIMeta& m, unsigned sessionId, SSRC ssrc,
                                         unsigned participantId, unsigned decodeTarget) {

            Log(Log::INFO) << "SwitchAgent: _onAPISetDecodeTarget: sessionId=" << sessionId
                           << ", ssrc=" << ssrc << ", participantId=" << participantId
                           << ", decodeTarget=" << decodeTarget << std::endl;

            json::json j;

            if (_adjustDecodeTarget(sessionId, ssrc, participantId, decodeTarget)) {
                j["type"] = "ok";
            } else {
                j["type"] = "error";
            }

            return j;
        }

        void _onAPIClose(const SwitchAPIMeta& m) {
            Log(Log::INFO) << "SwitchAgent: _onAPIClose: close" << std::endl;
        }

        #pragma mark etc:

        void _onTimer(Timer& t) {

            if (_dataPlane->totalStatistics().pkts > 0) {
                Log(Log::INFO) << "SwitchAgent: "
                               << "pkts=" << _dataPlane->totalStatistics().pkts << ", "
                               << "bytes=" << _dataPlane->totalStatistics().bytes << ", "
                               << "rtpPkts=" << _dataPlane->totalStatistics().rtpPkts << ", "
                               << "rtcpPkts=" << _dataPlane->totalStatistics().rtcpPkts << ", "
                               << "stunPkts=" << _dataPlane->totalStatistics().stunPkts << ", "
                               // << "frames=" << _dataPlane->totalStatistics().frames << ", "
                               << "av1SimpleDescriptors="
                               << _dataPlane->totalStatistics().av1SimpleDescriptors << ", "
                               << "av1ExtendedDescriptors="
                               << _dataPlane->totalStatistics().av1ExtendedDescriptors
                               << std::endl;
            }
        }
    };
}

#endif

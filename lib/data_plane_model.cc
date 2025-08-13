
#include "data_plane_model.h"

#include "proto/stun.h"
#include "proto/rtp.h"
#include "proto/rtcp.h"
#include "av1.h"
#include "log.h"

p4sfu::DataPlaneModel::DataPlaneModel(UDPInterface* udp)
    : DataPlane{},
      _udp{udp},
      _config{},
      _rtpDropDist{1, _config.rtpDropRate} {

    _udp->onMessage([this](UDPInterface& c, asio::ip::udp::endpoint& from, const char* buf,
                           std::size_t len) {

        this->_onPacket(c, from, (const unsigned char*) buf, len);
    });
}

p4sfu::DataPlaneModel::DataPlaneModel(boost::asio::io_context* io, DataPlane::Config* c)
    : DataPlane{io},
      _udp{new UDPServer{*io, reinterpret_cast<DataPlaneModel::Config*>(c)->port}},
      _config{*reinterpret_cast<DataPlaneModel::Config*>(c)},
      _rtpDropDist{1, _config.rtpDropRate} {

    _udp->onMessage([this](UDPInterface& c, asio::ip::udp::endpoint& from, const char* buf,
                           std::size_t len) {

        this->_onPacket(c, from, (const unsigned char*) buf, len);
    });
}

void p4sfu::DataPlaneModel::sendPacket(const PktOut& pkt) {

    asio::ip::udp::endpoint to{asio::ip::address_v4{pkt.to.ip().num()}, pkt.to.port()};
    _udp->sendTo(to, (const char*) pkt.buf, pkt.len);
}

void p4sfu::DataPlaneModel::addStream(const Stream& s) {

    Log(Log::INFO) << "DataPlaneModel: addStream: src=" << s.src << ", dst=" << s.dst
                   << ", ssrc=[" << s.ssrc << (s.rtxSsrc ? (","+std::to_string(s.rtxSsrc)) : "")
                   << "]" << std::endl;

    // matches and action for RTP and RTCP SR/SDES packets
    SFUTable::Match mainMatch{s.src, s.ssrc}, rtxMatch{s.src, s.rtxSsrc};

    _addMatch(mainMatch);

    if (s.rtxSsrc) {
        _addMatch(rtxMatch);
    }

    if (s.dst == net::IPv4Port{0, 0}) { // send stream-only, just add match, no forwarding rule
        return;
    }

    SFUTable::Action action{s.dst};
    // action.dropLayers.dropT2();

    // matches and action for RTCP packets going in reverse direction, only use single SSRC for now
    SFUTable::Match retMatch{s.dst, s.rtcpSsrc}, retRtxMatch{s.dst, s.rtcpRtxSsrc};
    _addMatch(retMatch);

    SFUTable::Action retAction{s.src};

    if (s.rtcpRtxSsrc) {
        _addMatch(retRtxMatch);
    }

    _addAction(mainMatch, _sfu[mainMatch], action);
    _addAction(retMatch, _sfu[retMatch], retAction);

    if (s.rtxSsrc) {
        _addAction(rtxMatch, _sfu[rtxMatch], action);
    }

    // don't add second action when sender's SSRC (for RTCP) is 1
    // would result in duplicate entries
    if (s.rtcpRtxSsrc && s.rtcpSsrc != 1) {
        _addAction(retRtxMatch, _sfu[retRtxMatch], retAction);
    }
}

void p4sfu::DataPlaneModel::removeStream(const Stream& s) {

    throw std::domain_error("DataPlaneModel: removeStream: not implemented");
}

void p4sfu::DataPlaneModel::adjustDecodeTarget(const net::IPv4Port& from, const net::IPv4Port& to,
                                               SSRC ssrc, const unsigned target) {

    if (_sfu.hasMatch(SFUTable::Match{from, ssrc})) {
        for (auto& entry = _sfu[SFUTable::Match{from, ssrc}]; auto& a: entry.actions()) {

            if (a.to() == to) {

                if (!a.svcConfig) {
                    a.svcConfig = av1::svc::L1T3{};
                }

                if (target > av1::svc::L1T3::MAX_IDENT) {
                    Log(Log::ERROR) << "DataPlaneModel: adjustDecodeTarget: invalid target: "
                                    << target << std::endl;
                    return;
                }

                a.svcConfig->decodeTarget = av1::svc::L1T3::decodeTargetFromNumIdentifier(target);

                Log(Log::INFO) << "DataPlaneModel: adjustDecodeTarget: decode target adjusted: "
                               << "from=" << from << ", to=" << to << ", ssrc=" << ssrc
                               << ", target=" << target << std::endl;

                return;
            }
        }

    } else {
        Log(Log::ERROR) << "DataPlaneModel: adjustDecodeTarget: no match for " << from
                        << ", ssrc=" << ssrc << std::endl;
    }
}

void p4sfu::DataPlaneModel::_addMatch(const SFUTable::Match& m) noexcept {

    if (!_sfu.hasMatch(m)) {
        _sfu.addMatch(m);
        Log(Log::INFO) << "DataPlaneModel: _addMatch: match added: addr=" << m.ipPort() << ", ssrc="
                       << m.ssrc() << std::endl;
    } else {
        Log(Log::DEBUG) << "DataPlaneModel: _addMatch: match already exists: addr=" << m.ipPort()
                        << ", ssrc=" << m.ssrc() << std::endl;
    }
}

void p4sfu::DataPlaneModel::_addAction(const SFUTable::Match& match, SFUTable::Entry& e,
                                       const SFUTable::Action& a) noexcept {

    if (!e.hasAction(a)) {
        e.addAction(a);
        Log(Log::INFO) << "DataPlaneModel: _addAction: action added: "
                       << "from=" << match.ipPort() << ", ssrc=" << match.ssrc() << ", "
                       << "to=" << a.to() << std::endl;
    } else {
        Log(Log::ERROR) << "DataPlaneModel: addStream: action already exists: "
                        << "from=" << match.ipPort() << ", ssrc=" << match.ssrc() << ", "
                        << "to=" << a.to() << std::endl;
    }
}

void p4sfu::DataPlaneModel::_onPacket(UDPInterface& c, asio::ip::udp::endpoint& from,
    const unsigned char* buf, std::size_t len) {

    if (stun::bufferContainsSTUN(buf, len)) {

        _totalStatistics.pkts++;
        _totalStatistics.bytes += len;
        _handleSTUN(net::IPv4Port{from.address().to_v4().to_string(), from.port()}, buf, len);

    } else if (rtp::contains_rtp_or_rtcp(buf, len)) {

        _totalStatistics.pkts++;
        _totalStatistics.bytes += len;

        auto* rtp = (rtp::hdr*) buf;

        if (rtp->is_rtcp()) {
            _handleRTCP(net::IPv4Port{from.address().to_v4().to_string(), from.port()}, buf, len);
        } else {
            _handleRTP(net::IPv4Port{from.address().to_v4().to_string(), from.port()}, buf, len);
        }
    }
}

void p4sfu::DataPlaneModel::_handleSTUN(const net::IPv4Port& from, const unsigned char* buf,
    std::size_t len) {

    _totalStatistics.stunPkts++;

    try {
        _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::stun, from, buf, len});
    } catch (std::bad_function_call& e) {
        throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
    }
}

void p4sfu::DataPlaneModel::_handleRTP(const net::IPv4Port& from, const unsigned char* buf,
    std::size_t len) {

    auto* rtp = (rtp::hdr*) buf;

    if (_rtpDropDist(_rand)) {
        Log(Log::INFO) << "DataPlaneModel: _handleRTP: randomly dropping packet: "
            << "ssrc=" << ntohl(rtp->ssrc) << ", seq=" << ntohs(rtp->seq) << std::endl;
        return;
    }

    _totalStatistics.rtpPkts++;

    auto* av1Ptr = rtp->extension_ptr(_config.av1RtpExt);
    std::optional<av1::DependencyDescriptor::MandatoryFields> av1;

    if (av1Ptr) {

        av1 = av1::DependencyDescriptor{av1Ptr + 1, rtp::ext_len(av1Ptr)}.mandatoryFields();

        if (rtp::ext_len(av1Ptr) > 3) {

            try {
                _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::av1, from, buf, len});
            } catch (std::bad_function_call& e) {
                throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
            }

            _totalStatistics.av1ExtendedDescriptors++;
        } else {
            _totalStatistics.av1SimpleDescriptors++;
        }
    }

    SFUTable::Match match{from, ntohl(rtp->ssrc)};

    if (_sfu.hasMatch(match)) {
        auto& actions = _sfu[match].actions();

        Log(Log::TRACE) << "DataPlaneModel: _handleRTP: packet match: from=" << from << ", ssrc="
                        << ntohl(rtp->ssrc) << ", actions=" << actions.size() <<  std::endl;

        for (auto& a: actions) {

            Log(Log::TRACE) << "  - action:" << std::endl;

            if (av1) { // handling for video frames with av1 descriptor

                // determine if packet needs to be dropped
                bool drop = a.svcConfig && a.svcConfig->drop(av1->templateId());

                // compute new sequence number
                auto seq = a.sequenceRewriter(av1->frameNumber(), ntohs(rtp->seq),
                                              av1->startOfFrame(), av1->endOfFrame(), drop);

                if (drop) {
                    Log(Log::TRACE) << "    - drop packet" << std::endl;
                    return; // drop the packet
                } else {
                    rtp->seq = htons(*seq); // set new sequence number
                    Log(Log::TRACE) << "    - rewrite seq " << ntohs(rtp->seq) << " -> " << *seq
                                    << std::endl;
                }
            }

            this->sendPacket(PktOut{a.to(), buf, len});
            Log(Log::TRACE) << "    - sent to " << a.to() << std::endl;
        }

    } else {
        Log(Log::WARN) << "DataPlaneModel: _handleRTP: no match for " << from << ", ssrc="
                       << ntohl(rtp->ssrc) << std::endl;
    }
}

void p4sfu::DataPlaneModel::_handleRTCP(const net::IPv4Port& from, const unsigned char* buf,
                                   std::size_t len) {

    _totalStatistics.rtcpPkts++;

    auto* rtcp = reinterpret_cast<const rtcp::hdr*>(buf);

    Log(Log::DEBUG) << "DataPlaneModel: _handleRTCP: pt="
                    << rtcp::pt_name(static_cast<rtcp::pt>(rtcp->pt)) << ", ssrc=" << std::dec
                    << ntohl(rtcp->sender_ssrc) << std::endl;

    // switch through outermost RTCP packet type
    switch (static_cast<rtcp::pt>(rtcp->pt)) {

        case rtcp::pt::sr: // usually combined with SDES
            _handleSR(from, buf, len);
            break;

        case rtcp::pt::rr: // usually combined with PSFB (REMB)
            _handleRR(from, buf, len);
            break;

        case rtcp::pt::rtpfb: // for NACK; usually combined with PSFB (REMB)
            _handleRTPFB(from, buf, len);
            break;

        case rtcp::pt::psfb: // for PLI; usually combined with additional PSFB (REMB)
            _handlePSFB(from, buf, len);
            break;

        case rtcp::pt::sdes:
            Log(Log::WARN) << "DataPlaneModel: _handleRTCP: logic for outermost RTCP SDES "
                           << "not implemented" << std::endl;
            break;

        default:
            Log(Log::WARN) << "DataPlaneModel: _handleRTCP: unknown RTCP type "
                           << static_cast<unsigned>(rtcp->pt) << std::endl;
    }
}

void p4sfu::DataPlaneModel::_handleSR(const net::IPv4Port& from, const unsigned char* buf,
                                      std::size_t len) {

    // SRs are handled exclusively in the data plane and forwarded in the same manner as
    // RTP packets

    auto* rtcp = (rtcp::hdr*) buf;

    /*
    if (rtcp->pt == 200) {
        std::cout << "RTCP SR" << std::endl;
    }

    if (rtcp->byte_len() < len) {
        auto* rtcp2 = (rtcp::hdr*) (buf + rtcp->byte_len());
        if (rtcp2->pt == 202) {
            std::cout << "RTCP SDES" << std::endl;
        }
    }
    */

    SFUTable::Match match{from, ntohl(rtcp->sender_ssrc)};

    if (_sfu.hasMatch(match)) {

        auto& actions = _sfu[match].actions();

        Log(Log::DEBUG) << "DataPlaneModel: _handleRTCP: sr packet match: from=" << from
                        << ", ssrc=" << ntohl(rtcp->sender_ssrc) << ", actions="
                        << actions.size() << std::endl;

        for (auto& a: actions) { // send SRs to all receivers
            this->sendPacket(PktOut{a.to(), buf, len});
            Log(Log::DEBUG) << "  - sent to " << a.to() << std::endl;
        }

    } else {
        Log(Log::WARN) << "DataPlaneModel: _handleRTCP: no match for sr: from=" << from << ", ssrc="
                       << ntohl(rtcp->sender_ssrc) << std::endl;
    }

    /*
    Log(Log::DEBUG) << "  - sr: ntp_ts_msw=" << std::dec << ntohl(rtcp->msg.sr.ntp_ts_msw)
                    << ", ntp_ts_lsw=" << std::dec << ntohl(rtcp->msg.sr.ntp_ts_lsw)
                    << ", rtp_ts=" << std::dec << ntohl(rtcp->msg.sr.rtp_ts)
                    << ", sender_pkt_count=" << std::dec
                    << ntohl(rtcp->msg.sr.sender_pkt_count)
                    << ", sender_byte_count=" << std::dec
                    << ntohl(rtcp->msg.sr.sender_byte_count)
                    << std::endl;
    */
}

void p4sfu::DataPlaneModel::_handleRR(const net::IPv4Port& from, const unsigned char* buf,
                                      std::size_t len) {

    // RRs are passed to the switch agent and exclusively handled there
    // - a future optimization might inspect bandwidth estimates and only forward to control plane
    //   if the REMB value changed (at all or beyond threshold)

    auto* rtcp = reinterpret_cast<const rtcp::hdr*>(buf);

    Log(Log::DEBUG) << "DataPlaneModel: _handleRTCP: rr packet match: from=" << from
                    << ", ssrc=" << ntohl(rtcp->sender_ssrc) << std::endl;

    /*
    std::cout << "RTCP RR" << std::endl;

    for (auto i = 0; i < rtcp->recep_rep_count(); i++) {
        std::cout << "  - " << ntohl(rtcp->data.rr[i].ssrc) << ": frac_lost="
                  << (double) rtcp->data.rr[i].frac_lost() / 255 << ", jitter="
                  << ntohl(rtcp->data.rr[i].jitter) << std::endl;
    }

    if (rtcp->byte_len() < len) {
        auto* rtcp2 = (rtcp::hdr*) (buf + rtcp->byte_len());

        if (rtcp2->pt == 206 && rtcp2->fb_fmt() == 15) {
            std::cout << "RTCP REMB" << std::endl;

            for (int i = 0; i < rtcp2->data.remb.num_ssrcs(); i++) {
                std::cout << "  - " << std::dec << ntohl(rtcp2->data.remb.ssrcs[i])  << ": "
                          << "bit_rate=" << rtcp2->data.remb.bit_rate() << std::endl;
            }
        }
    }
    */

    /*
    Log(Log::DEBUG) << "  - rr: rc=" << rtcp->recep_rep_count()
                    << ", ssrc=" << std::dec << ntohl(rtcp->msg.rr.ssrc)
                    << ", frac_lost=" << std::dec << ntohl(rtcp->msg.rr.frac_lost())
                    << ", cum_lost=" << std::dec << ntohl(rtcp->msg.rr.cum_lost())
                    << ", ext_highest_seq_num=" << std::dec
                    << ntohl(rtcp->msg.rr.ext_highest_seq_num)
                    << ", jitter=" << std::dec << ntohl(rtcp->msg.rr.jitter)
                    << ", lsr=" << std::dec << ntohl(rtcp->msg.rr.lsr)
                    << ", dlsr=" << std::dec << ntohl(rtcp->msg.rr.dlsr)
    */

    try {
        _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::rtcp, from, buf, len});
    } catch (std::bad_function_call& e) {
        throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
    }
}

void p4sfu::DataPlaneModel::_handleRTPFB(const net::IPv4Port& from, const unsigned char* buf,
                                         std::size_t len) {

    auto* rtcp = (rtcp::hdr*) buf;

    Log(Log::DEBUG) << "DataPlaneModel: _handleRTPFB: from=" << from << ", ssrc="
                    << ntohl(rtcp->sender_ssrc) << std::endl;

    if (rtcp->fb_fmt() == 1) { // NACK

        Log(Log::DEBUG) << "  - NACK: ssrc=" << ntohl(rtcp->data.nack.ssrc)
                        << ", seq=" << ntohs(rtcp->data.nack.pid)
                        << ", blp=" << ntohs(rtcp->data.nack.blp) << std::endl;

        // send to media sender:

        if (_sfu.hasMatch(SFUTable::Match{from, ntohl(rtcp->sender_ssrc)})) {

            auto& entry = _sfu[SFUTable::Match{from, ntohl(rtcp->sender_ssrc)}];

            Log(Log::DEBUG) << "DataPlaneModel: _handleRTPFB: packet match: from=" << from
                            << ", ssrc=" << ntohl(rtcp->sender_ssrc) << ", actions="
                            << entry.actions().size() <<  std::endl;

            for (auto& action: entry.actions()) {
                this->sendPacket(PktOut{action.to(), buf, len});
                Log(Log::DEBUG) << "  - action:" << std::endl;
                Log(Log::DEBUG) << "    - sent to " << action.to() << std::endl;
            }

        } else {
            Log(Log::WARN) << "DataPlaneModel: _handleRTPFB: no match for NACK: from=" << from
                           << ", ssrc=" << ntohl(rtcp->sender_ssrc) << std::endl;
        }

        // send copy to switch agent:

        try {
            _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::rtcp, from, buf, len});
        } catch (std::bad_function_call& e) {
            throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
        }

    } else {
        Log(Log::WARN) << "DataPlaneModel: _handleRTPFB: logic to handle RTPFB other than NACK "
                       << "not implemented" << std::endl;
    }
}

void  p4sfu::DataPlaneModel::_handlePSFB(const net::IPv4Port& from, const unsigned char* buf,
                                         std::size_t len) {

    auto* rtcp = (rtcp::hdr*) buf;

    Log(Log::DEBUG) << "DataPlaneModel: _handlePSFB: from=" << from << ", ssrc="
                    << ntohl(rtcp->sender_ssrc) << std::endl;

    if (rtcp->fb_fmt() == 1) { // PLI
        Log(Log::DEBUG) << "  - PLI: ssrc=" << ntohl(rtcp->data.pli.ssrc) << std::endl;

        // send to media sender:

        if (_sfu.hasMatch(SFUTable::Match{from, ntohl(rtcp->sender_ssrc)})) {

            auto& entry = _sfu[SFUTable::Match{from, ntohl(rtcp->sender_ssrc)}];

            Log(Log::DEBUG) << "DataPlaneModel: _handlePSFB: packet match: from=" << from
                            << ", ssrc=" << ntohl(rtcp->sender_ssrc) << ", actions="
                            << entry.actions().size() <<  std::endl;

            for (auto& action: entry.actions()) {
                this->sendPacket(PktOut{action.to(), buf, len});
                Log(Log::DEBUG) << "  - action:" << std::endl;
                Log(Log::DEBUG) << "    - sent to " << action.to() << std::endl;
            }

        } else {
            Log(Log::WARN) << "DataPlaneModel: _handlePSFB: no match for PLI: from=" << from
                           << ", ssrc=" << ntohl(rtcp->sender_ssrc) << std::endl;
        }

        // send copy to switch agent:

        try {
            _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::rtcp, from, buf, len});
        } catch (std::bad_function_call& e) {
            throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
        }

    } else {
        Log(Log::WARN) << "DataPlaneModel: _handlePSFB: logic to handle PSFB other than PLI "
                       << "not implemented" << std::endl;
    }
}

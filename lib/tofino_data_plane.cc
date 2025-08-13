
#include <nlohmann/json.hpp>

#include "net/net.h"
#include "tofino_data_plane.h"
#include "log.h"
#include "proto/stun.h"
#include "proto/rtp.h"

namespace json = nlohmann;

p4sfu::TofinoDataPlane::TofinoDataPlane(boost::asio::io_context* io, DataPlane::Config* c)
    : DataPlane(io),
      _config(*reinterpret_cast<TofinoDataPlane::Config*>(c)),
      _server(*_io, _config.tofinoListenPort),
      _dataPlaneInterface(_config.dataPlaneIface),
      _dataPlaneInterfaceFd(*_io, _dataPlaneInterface.fd()) {

    Log::config = { .level = _config.verbose ? Log::DEBUG : Log::INFO, .printLabel = true };

    Log(Log::INFO) << "TofinoDataPlane: TofinoDataPlane: tofino-listen-port="
                   << _config.tofinoListenPort << ", data-plane-iface="
                   << _config.dataPlaneIface << std::endl;

    _server.onOpen([this](auto&&... args) -> void {
         _onSwitchOpen(std::forward<decltype(args)>(args)...);
    });

    _server.onMessage([this](auto&&... args) -> void {
         _onSwitchMessage(std::forward<decltype(args)>(args)...);
    });

    _server.onClose([this](auto&&... args) -> void {
         _onSwitchClose(std::forward<decltype(args)>(args)...);
    });

    _dataPlaneInterfaceFd.onReadReady([this](auto&&... args) -> void {
        _onDataPlanePacket(std::forward<decltype(args)>(args)...);
    });
}

void p4sfu::TofinoDataPlane::sendPacket(const p4sfu::DataPlane::PktOut& pkt) {

    Log(Log::DEBUG) << "TofinoDataPlane: sendPacket: received pkt-out: to=" << pkt.to
                    << ", l7_bytes=" << util::hexString(pkt.buf, pkt.len, 4) << std::endl;

    unsigned char buf[1024] = {0};
    unsigned offset = 0;

    auto* eth = (net::eth::hdr*) (buf);
    eth->init_default();
    eth->ether_type = htons(static_cast<std::uint16_t>(net::eth::type::ipv4));
    offset += net::eth::HDR_LEN;

    auto* ip  = (net::ipv4::hdr*) (buf + offset);
    ip->init_default();
    ip->total_length  = htons(pkt.len + net::ipv4::HDR_LEN + net::udp::HDR_LEN);
    ip->next_proto_id = static_cast<std::uint8_t>(net::ipv4::proto::udp);
    ip->hdr_checksum  = 0; // can be computed in Tofino (chksum is only over header)
    ip->src_addr      = htonl(net::IPv4{"10.0.211.111"}.num()); //TODO: read from config
    ip->dst_addr      = htonl(pkt.to.ip().num());
    offset += net::ipv4::HDR_LEN;

    auto* udp = (net::udp::hdr*) (buf + offset);
    udp->src_port     = htons(3000); //TODO: read from config
    udp->dst_port     = htons(pkt.to.port());
    udp->dgram_len    = htons(pkt.len + net::udp::HDR_LEN);
    udp->dgram_cksum  = 0; // udp chksum is optional
    offset += net::udp::HDR_LEN;

    std::memcpy((buf + offset), pkt.buf, pkt.len);

    _dataPlaneInterface.inject(buf, pkt.len + offset);

    Log(Log::DEBUG) << "TofinoDataPlane: sendPacket: sent pkt-out: len=" << pkt.len + offset
                    << ", bytes=" << util::hexString(buf, pkt.len + offset, 4) << std::endl;
}

void p4sfu::TofinoDataPlane::addStream(const p4sfu::DataPlane::Stream& s) {

    Log(Log::INFO) << "TofinoDataPlane: addStream: src=" << s.src << ", dst=" << s.dst
                   << ", ssrc=" << s.ssrc << std::endl;

    if (_tofinoConnection == nullptr) {
        Log(Log::ERROR) << "TofinoDataPlane: addStream: no switch connected" << std::endl;
        return;
    }

    json::json j;
    j["api"] = "add_stream";
    j["sip"] = s.src.ip().str();
    j["sport"] = s.src.port();
    j["ssrc"] = s.ssrc;
    j["ssrc_retx"] = s.rtxSsrc;
    j["dip"] = s.dst.ip().str();
    j["dport"] = s.dst.port();
    // do not include eport in message (set by Tofino directly, discussed w/ Sata 05/06/24):
    //j["eport"] = s.egressPort;

    Log(Log::DEBUG) << "TofinoDataPlane: addStream: sent add_stream message: ssrc=" << s.ssrc
                    << ", msg=" << j.dump() << std::endl;

    _tofinoConnection->write(j.dump());
}

void p4sfu::TofinoDataPlane::removeStream(const p4sfu::DataPlane::Stream& s) {

    Log(Log::INFO) << "TofinoDataPlane: removeStream" << std::endl;

    if (!_tofinoConnection) {
        Log(Log::ERROR) << "TofinoDataPlane: removeStream: no switch connected" << std::endl;
        return;
    }

    /*
    json::json j;
    j["api"] = "remove_stream";
    j["sip"] = s.src.ip().str();
    j["sport"] = s.src.port();
    j["ssrc"] = s.ssrc;
    j["ssrc_rtx"] = s.rtxSsrc;
    j["dip"] = s.dst.ip().str();
    j["dport"] = s.dst.port();

    _tofinoConnection->write(j.dump());

         Log(Log::DEBUG) << "TofinoDataPlane: removeStream: sent remove_stream message: ssrc="
                         << s.ssrc << std::endl;
    */
}

void p4sfu::TofinoDataPlane::adjustDecodeTarget(const net::IPv4Port& from, const net::IPv4Port& to,
                                                const SSRC ssrc, const unsigned target) {

    Log(Log::INFO) << "TofinoDataPlane: adjustDecodeTarget" << std::endl;
    // Log(Log::INFO) << "TofinoDataPlane:   - not implemented" << std::endl;

    if (!_tofinoConnection) {
        Log(Log::ERROR) << "TofinoDataPlane: adjustDecodeTarget: no switch connected" << std::endl;
        return;
    }

    // disable_av1_enhancement_layer(sip, sport, ssrc, ssrc_retx, dip, dport)

    json::json j;
    j["api"] = "disable_av1_enhancement_layer";
    j["sip"] = from.ip().str();
    j["sport"] = from.port();
    j["ssrc"] = ssrc;
    //TODO: remove this from API call:
    j["ssrc_retx"] = 0;
    j["dip"] = to.ip().str();
    j["dport"] = to.port();

    std::cout << j.dump() << std::endl;
    _tofinoConnection->write(j.dump());
}

void p4sfu::TofinoDataPlane::_onSwitchOpen(WebSocketSession<TofinoMeta>& s) {

    Log(Log::INFO) << "TofinoDataPlane: _onSwitchOpen: remote=" << s.remote() << std::endl;

    _tofinoConnection = s.shared_from_this();
    s.start();


    /* send test instructions to tofino_pre.py
    this->addStream(Stream{
        .src = net::IPv4Port{net::IPv4{"0.0.0.0"}, 1},
        .dst = net::IPv4Port{net::IPv4{"1.1.1.1"}, 2},
        .ssrc = 3,
        .rtxSsrc = 4,
        .egressPort = 5
    });

    this->removeStream(Stream{
        .src = net::IPv4Port{net::IPv4{"0.0.0.0"}, 1},
        .dst = net::IPv4Port{net::IPv4{"1.1.1.1"}, 2},
        .ssrc = 3,
        .rtxSsrc = 4
    });
    */
}

void p4sfu::TofinoDataPlane::_onSwitchMessage(WebSocketSession<TofinoMeta>& s, const char* m,
                                              std::size_t l) {

    Log(Log::DEBUG) << "TofinoDataPlane: _onSwitchMessage: len=" << l << std::endl;
}

void p4sfu::TofinoDataPlane::_onSwitchClose(WebSocketSession<TofinoMeta>& s) {

    //_tofinoConnection.reset();
    Log(Log::WARN) << "TofinoDataPlane: _onSwitchClose: remote=" << s.remote()
                   << ", removed switch." << std::endl;
}

void p4sfu::TofinoDataPlane::_onDataPlanePacket(FileDescriptor& fd) {

    pcap_pkt pkt;

    if (_dataPlaneInterface.next(pkt)) {

        Log(Log::DEBUG) << "TofinoDataPlane: _onDataPlanePacket: fd=" << fd.fd() << ", frame_len="
                        << pkt.frame_len << ", bytes=" << util::hexString(pkt.buf, pkt.frame_len, 4)
                        << std::endl;

        // for testing: set to 4 when sniffing from loopback
        unsigned offset = 0;

        const auto* eth = (const net::eth::hdr*)(pkt.buf + offset);

        if (static_cast<net::eth::type>(ntohs(eth->ether_type)) == net::eth::type::ipv4) {

            const auto* ip = (const net::ipv4::hdr*)(pkt.buf + net::eth::HDR_LEN + offset);

            if ((net::ipv4::proto)(ip->next_proto_id) == net::ipv4::proto::udp) {

                const auto* udp =
                    (const net::udp::hdr*)(pkt.buf + net::eth::HDR_LEN + ip->ihl_bytes() + offset);

                net::IPv4Port pktInFrom{net::IPv4{ntohl(ip->src_addr)}, ntohs(udp->src_port)};
                auto hdrLen = net::eth::HDR_LEN + ip->ihl_bytes() + net::udp::HDR_LEN;
                auto payloadLen = pkt.frame_len - hdrLen - offset;
                const auto* payload = (const unsigned char*)(pkt.buf + hdrLen + offset);

                if (stun::bufferContainsSTUN(payload, payloadLen)) {

                    _handleSTUN(pktInFrom, payload, payloadLen);

                } else if (rtp::contains_rtp_or_rtcp(payload, payloadLen)) {

                    auto* rtp = (rtp::hdr*) payload;

                    if (rtp->is_rtcp()) {
                        _handleRTCP(pktInFrom, payload, payloadLen);
                    } else {
                        _handleRTP(pktInFrom, payload, payloadLen);
                    }
                } else {
                    Log(Log::ERROR) << "TofinoDataPlane: _onDataPlanePacket: unknown payload"
                                    << std::endl;
                }
            }
        }

    } else {
        Log(Log::ERROR) << "TofinoDataPlane: _onDataPlanePacket: failed pcap_interface::next"
                        << std::endl;
    }
}

void p4sfu::TofinoDataPlane::_handleSTUN(const net::IPv4Port& from, const unsigned char* buf,
                                         std::size_t len) {

    Log(Log::DEBUG) << "TofinoDataPlane: _handleSTUN: from=" << from << std::endl;

    try {
        _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::stun, from, buf, len});
    } catch (std::bad_function_call& e) {
        throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
    }
}

void p4sfu::TofinoDataPlane::_handleRTP(const net::IPv4Port& from, const unsigned char* buf,
                                        std::size_t len) {

    Log(Log::DEBUG) << "TofinoDataPlane: _handleRTP: from=" << from << std::endl;
    Log(Log::DEBUG) << "TofinoDataPlane:   - not implemented" << std::endl;

    /*
    try {
        _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::rtp, from, buf, len});
    } catch (std::bad_function_call& e) {
        throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
    }
    */
}

void p4sfu::TofinoDataPlane::_handleRTCP(const net::IPv4Port& from, const unsigned char* buf,
                                         std::size_t len) {

    Log(Log::DEBUG) << "TofinoDataPlane: _handleRTCP: from=" << from << std::endl;
    Log(Log::DEBUG) << "TofinoDataPlane:   - not implemented" << std::endl;

    /*
    try {
        _controlPlanePacketHandler(*this, PktIn{PktIn::Reason::rtcp, from, buf, len});
    } catch (std::bad_function_call& e) {
        throw std::logic_error("DataPlaneModel: no control-plane packet handler set");
    }
    */
}

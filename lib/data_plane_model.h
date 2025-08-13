#ifndef P4SFU_DATA_PLANE_MODEL_H
#define P4SFU_DATA_PLANE_MODEL_H

#include <boost/asio.hpp>
#include <random>
#include "data_plane.h"
#include "net/udp_server.h"
#include "sfu_table.h"
#include "av1.h"
#include "proto/rtp.h"

using namespace boost;

namespace p4sfu {

    class DataPlaneModel : public DataPlane {
    public:

        struct Config : public DataPlane::Config {
            //! RTP extension identifier for AV1 dependency descriptor
            unsigned av1RtpExt;
            //! UDP port the SFU data plane uses for RTP traffic
            unsigned short port;
            double rtpDropRate = 0;
        };

        struct RTPPktModifications {
            std::optional<std::uint16_t> rtpSeq = std::nullopt;
            std::optional<rtp::ext> av1 = std::nullopt;
        };

        explicit DataPlaneModel(UDPInterface* udp);
        explicit DataPlaneModel(boost::asio::io_context* io, DataPlane::Config* c);

        // from abstract DataPlane:
        void sendPacket(const PktOut& pkt) override;
        void addStream(const Stream& s) override;
        void removeStream(const Stream& s) override;
        void adjustDecodeTarget(const net::IPv4Port& from, const net::IPv4Port& to, SSRC ssrc,
                                unsigned target) override;

    private:

        //! adds a match in the sfu table, if it doesn't already exist
        void _addMatch(const SFUTable::Match& m) noexcept;

        //! adds an action associated with an entry in the sfu table if it doesn't already exist
        void _addAction(const SFUTable::Match& match, SFUTable::Entry& e,
                        const SFUTable::Action& a) noexcept;

        void _onPacket(UDPInterface& c, asio::ip::udp::endpoint& from, const unsigned char* buf,
                       std::size_t len);

        void _handleSTUN(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRTP(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRTCP(const net::IPv4Port& fromm, const unsigned char* buf, std::size_t len);
        void _handleSR(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRR(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRTPFB(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handlePSFB(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);

        UDPInterface* _udp = nullptr;
        SFUTable _sfu;
        DataPlaneModel::Config _config;
        std::mt19937 _rand = std::mt19937(std::random_device()());
        std::binomial_distribution<> _rtpDropDist;
    };
}

#endif

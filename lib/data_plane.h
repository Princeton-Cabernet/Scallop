#ifndef P4SFU_DATA_PLANE_H
#define P4SFU_DATA_PLANE_H

#include "switch_statistics.h"
#include "net/net.h"
#include "p4sfu.h"

#include <boost/asio.hpp>

namespace p4sfu {
    class DataPlane {
    public:

        struct PktIn {

            enum class Reason {
                stun = 0,
                av1  = 1,
                rtcp = 2
            } reason;

            net::IPv4Port                   from;
            const unsigned char*            buf;
            std::size_t                     len;
        };

        struct PktOut {
            net::IPv4Port                   to;
            const unsigned char*            buf;
            std::size_t                     len;
        };

        struct Stream {
            net::IPv4Port src;
            net::IPv4Port dst;
            std::uint32_t ssrc;
            std::uint32_t rtxSsrc;
            std::uint32_t rtcpSsrc;
            std::uint32_t rtcpRtxSsrc;
            std::uint16_t egressPort;
        };

        struct Config { };

        DataPlane() = default;

        explicit DataPlane(boost::asio::io_context* io)
            : _io(io) { }

        virtual void sendPacket(const PktOut&) = 0;

        void onPacketToController(std::function<void (DataPlane&, PktIn)>&& f) {
            _controlPlanePacketHandler = f;
        }

        [[nodiscard]] const TotalPacketStatistics& totalStatistics() const {
            return _totalStatistics;
        }

        virtual void addStream(const Stream& s) = 0;
        virtual void removeStream(const Stream& s) = 0;
        virtual void adjustDecodeTarget(const net::IPv4Port& from, const net::IPv4Port& to,
                                        SSRC ssrc, unsigned target) = 0;

        virtual ~DataPlane() = default;

    protected:
        boost::asio::io_context* _io = nullptr;
        std::function<void (DataPlane&, PktIn)> _controlPlanePacketHandler;
        TotalPacketStatistics _totalStatistics;
    };
}

#endif

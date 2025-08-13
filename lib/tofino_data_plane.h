#ifndef P4SFU_TOFINO_DATA_PLANE_H
#define P4SFU_TOFINO_DATA_PLANE_H

#include <boost/asio.hpp>
#include "data_plane.h"
#include "file_descriptor.h"
#include "net/pcap_interface.h"
#include "net/web_socket_server.h"
#include "net/web_socket_session.h"

namespace p4sfu {

    class TofinoDataPlane : public DataPlane {

        struct TofinoMeta { };

    public:
        struct Config : public DataPlane::Config {
            bool verbose                    = false;
            unsigned short tofinoListenPort = 8765;
            std::string dataPlaneIface;
        };

        explicit TofinoDataPlane(boost::asio::io_context* io, DataPlane::Config* c);
        void sendPacket(const PktOut& pkt) override;

        void addStream(const Stream& s) override;
        void removeStream(const Stream& s) override;
        void adjustDecodeTarget(const net::IPv4Port& from, const net::IPv4Port& to, SSRC ssrc,
                                unsigned target) override;

    private:
        void _onSwitchOpen(WebSocketSession<TofinoMeta>& s);
        void _onSwitchMessage(WebSocketSession<TofinoMeta>& s, const char* m, std::size_t l);
        void _onSwitchClose(WebSocketSession<TofinoMeta>& s);
        
        void _onDataPlanePacket(FileDescriptor& fd);
        void _handleSTUN(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRTP(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);
        void _handleRTCP(const net::IPv4Port& from, const unsigned char* buf, std::size_t len);

        Config _config;
        WebSocketServer<WebSocketSession<TofinoMeta>> _server;
        std::shared_ptr<WebSocketSession<TofinoMeta>> _tofinoConnection;
        PCAPInterface _dataPlaneInterface;
        FileDescriptor _dataPlaneInterfaceFd;
    };
}

#endif

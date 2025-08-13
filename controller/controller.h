
#ifndef P4_SFU_CONTROLLER_H
#define P4_SFU_CONTROLLER_H

#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

#include "../lib/controller_api.h"
#include "../lib/controller_client_connection.h"
#include "../lib/controller_client_interface.h"
#include "../lib/controller_switch_connection.h"
#include "../lib/controller_switch_interface.h"
#include "../lib/controller_web_server.h"
#include "../lib/net/tcp_server.h"
#include "../lib/net/web_socket_server.h"
#include "../lib/rpc.h"
#include "../lib/session.h"
#include "../lib/session_manager.h"
#include "../lib/timer.h"
#include "../lib/util.h"

namespace json = nlohmann;
using namespace boost;

namespace p4sfu {

    class Controller {

    public:

        struct Config {
            std::uint16_t clientSidePort;
            std::uint16_t switchSidePort;
            std::string   dataPlaneIPv4;
            std::uint16_t dataPlanePort;
            std::uint16_t apiListenPort;
            std::uint16_t webPort;
            std::string   certFile;
            std::string   keyFile;
            std::string   limitNetwork;
            std::uint8_t  limitMask;
            bool          verbose;
        };

        Controller() = delete;
        explicit Controller(const Config& c);
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

        int operator()();

    private:

        void _onClientOpen(ControllerClientConnection& c);
        void _onClientHello(ControllerClientConnection& c, const rpc::cl::Hello& m);
        void _onNewParticipant(ControllerClientConnection& c, const rpc::cl::Hello& m);
        void _onClientOffer(ControllerClientConnection& c, const rpc::cl::Offer& m);
        void _onClientAnswer(ControllerClientConnection& c, const rpc::cl::Answer& m);
        void _onClientClose(const ClientMeta& m);

        void _onSwitchOpen(ControllerSwitchConnection&);
        void _onSwitchHello(ControllerSwitchConnection&, const rpc::sw::Hello&);
        void _onNewSwitch(ControllerSwitchConnection&, const rpc::sw::Hello&);
        void _onSwitchClose(const SwitchMeta&);

        json::json _onAPIGetSessions();
        json::json _onAPIGetSession(unsigned id);

        void _onNewStream(const Session& session, unsigned participantId, const Stream& stream);

        void _onTimer(Timer& t);

        Config _config;

        asio::io_context _io;

        struct _ssl {
            _ssl(const std::string& certFile, const std::string& keyFile)
                : ssl{asio::ssl::context::tlsv12} {

                auto cert = util::readFile(certFile);
                auto key = util::readFile(keyFile);

                ssl.set_options(boost::asio::ssl::context::default_workarounds |
                                boost::asio::ssl::context::no_sslv2 |
                                boost::asio::ssl::context::single_dh_use);

                ssl.use_certificate_chain(boost::asio::buffer(cert.data(), cert.size()));

                ssl.use_private_key(boost::asio::buffer(key.data(), key.size()),
                                    boost::asio::ssl::context::file_format::pem);
            }

            asio::ssl::context ssl;
        } _ssl;

        ControllerClientInterface _clients;
        ControllerSwitchInterface _switches;
        ControllerAPI _api;
        ControllerWebServer _web;

        Timer _timer;
        SessionManager _sessions;
        SFUConfig _sfuConfig;

        const unsigned ALLOWED_SWITCH_ID = 0;
    };
}

#endif

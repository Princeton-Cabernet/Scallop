
#ifndef P4SFU_API_H
#define P4SFU_API_H

#include <map>
#include "net/web_socket_server.h"
#include "net/web_socket_session.h"

#include "log.h"

namespace p4sfu {

    template <typename Meta>
    class API {
    public:

        explicit API(asio::io_context &io, unsigned short port)
            : _server(io, port) {

            _server.onOpen([this](auto&&... args) {
                _onOpen(std::forward<decltype(args)>(args)...);
            });

            _server.onMessage([this](auto&&... args) {
                _onMessage(std::forward<decltype(args)>(args)...);
            });

            _server.onClose([this](auto&&... args) {
                _onClose(std::forward<decltype(args)>(args)...);
            });
        }

        class Connection {
        public:
            Connection() = delete;
            Connection(const Connection&) = default;
            Connection& operator=(const Connection&) = default;
            explicit Connection(WebSocketSession<Meta>& s) { }

        private:
            std::shared_ptr<WebSocketSession<Meta>> _ws;
        };

        std::function<void (Connection&)> onOpen;
        std::function<void (Meta&)> onClose;

    protected:

        virtual void _onOpen(WebSocketSession<Meta>& s) {

            s.start();

            auto [it, success] = _connections.emplace(s.remote(), Connection{s});

            if (success) {
                onOpen(it->second);
            } else {
                Log(Log::ERROR) << "API: _onOpen: failed adding new connection" << std::endl;
            }
        }

        virtual void _onMessage(WebSocketSession<Meta>& s, const char* buf, std::size_t len) = 0;

        virtual void _onClose(WebSocketSession<Meta>& s) const {
            onClose(s.meta());
        }

        WebSocketServer<WebSocketSession<Meta>> _server;
        std::map<asio::ip::tcp::endpoint, Connection> _connections;
    };
}

#endif

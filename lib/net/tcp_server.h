
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <boost/asio.hpp>
#include <optional>
#include "tcp_session.h"

using namespace boost;

template <typename SessionType>
class TCPServer {

    friend class _BaseTCPSession;

public:

    TCPServer(asio::io_context& io, unsigned short port)
        : _acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
          _socket(io) {

        _accept();
    }

    void onOpen(typename SessionType::OnOpenHandler&& f) {
        _onOpen = std::move(f);
    }

    void onMessage(typename SessionType::OnMessageHandler&& f) {
        _onMessage = std::move(f);
    }

    void onClose(typename SessionType::OnCloseHandler&& f) {
        _onClose = std::move(f);
    }

private:

    void _accept() {

        _acceptor.async_accept(_socket, [this](std::error_code ec) {

            auto s = std::make_shared<SessionType>(std::move(_socket));
            s->_onMessage = _onMessage;
            s->_onClose = _onClose;

            if (_onOpen) {
                (*_onOpen)(*s);
            } else {
                s->start();
            }

            _accept();
        });
    }

    std::optional<typename SessionType::OnOpenHandler> _onOpen = std::nullopt;
    std::optional<typename SessionType::OnMessageHandler> _onMessage = std::nullopt;
    std::optional<typename SessionType::OnCloseHandler> _onClose = std::nullopt;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::tcp::socket _socket;
};

#endif

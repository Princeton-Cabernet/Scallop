
#ifndef SECURE_WEB_SOCKET_SERVER_H
#define SECURE_WEB_SOCKET_SERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <optional>
#include <iostream>

#include "web_socket_session.h"

using namespace boost;

template <typename SessionType>
class SecureWebSocketServer {

public:

    SecureWebSocketServer(asio::io_context& ioContext, unsigned short port, asio::ssl::context& ssl)
            : _acceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
              _socket(ioContext),
              _ssl{ssl} {

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

            auto s = std::make_shared<SessionType>(std::move(_socket), _ssl);
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
    asio::ssl::context& _ssl;
};

#endif

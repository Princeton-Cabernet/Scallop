
#ifndef SECURE_WEB_SOCKET_SESSION_H
#define SECURE_WEB_SOCKET_SESSION_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <optional>
#include <iostream>

using namespace boost;

template <typename Meta>
class SecureWebSocketSession : public std::enable_shared_from_this<SecureWebSocketSession<Meta>> {

    template <typename SessionType>
    friend class SecureWebSocketServer;

public:
    typedef std::function<void (SecureWebSocketSession&, char*, std::size_t)> OnMessageHandler;
    typedef std::function<void (SecureWebSocketSession&)> OnOpenHandler;
    typedef std::function<void (SecureWebSocketSession&)> OnCloseHandler;

    SecureWebSocketSession(asio::ip::tcp::socket socket, asio::ssl::context& ssl)
        : _remote(socket.remote_endpoint()),
          _wsStream(std::move(socket), ssl),
          _ssl{ssl} {

        _wsStream.next_layer().handshake(asio::ssl::stream_base::server);

        _wsStream.set_option(beast::websocket::stream_base::decorator(
            [](beast::websocket::response_type& res) {
                res.set(beast::http::field::server,"p4-sfu controller");
            }));

        _wsStream.set_option(
                beast::websocket::stream_base::timeout::suggested(
                        beast::role_type::server));

        _wsStream.accept();
    }

    void start() {
        _read();
    }

    void write(const std::string& data) {

        auto remainingBytes = data.length();
        system::error_code ec;

        while (remainingBytes > 0) {

            // write data in increments of 1024 bytes:
            auto len = std::min(remainingBytes, 1024ul);

            _wsStream.write(asio::buffer(data.substr(data.length() - remainingBytes, len), len), ec);
            remainingBytes -= len;

            if (ec) {
                throw std::runtime_error("SecureWebSocketSession: write: failed: " +
                    std::to_string(ec.value()));
            }
        }
    }

    [[nodiscard]] const asio::ip::tcp::endpoint& remote() const {
        return _remote;
    }

    Meta& meta() {
        return _meta;
    }

    ~SecureWebSocketSession() = default;

private:

    void _read() {

        _wsStream.async_read(_readBuf, [this, self = this->shared_from_this()]
            (system::error_code ec, std::size_t len) {

           if (!ec) {

               if (_onMessage) {
                   (*_onMessage)(*this, static_cast<char*>(_readBuf.data().data()), len);
               }

               _readBuf.consume(len);

               _read();

           } else {

               if(ec.value() == static_cast<int>(beast::websocket::error::closed)
                  || ec.value() == static_cast<int>(asio::error::broken_pipe)
                  || ec.value() == static_cast<int>(asio::error::connection_reset)
                  || ec.value() == static_cast<int>(asio::error::eof)
                  || ec.value() == static_cast<int>(asio::error::not_connected)) {

                   if (_onClose) {
                       (*_onClose)(*this);
                   }

               } else {
                   throw std::runtime_error("SecureWebSocketSession: _read() failed: "
                                            + std::to_string(ec.value()) + ": " + ec.message());
               }
           }
        });
    }

    Meta _meta;
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
    std::optional<OnCloseHandler> _onClose = std::nullopt;
    asio::ip::tcp::endpoint _remote;
    asio::ssl::context& _ssl;
    beast::websocket::stream<asio::ssl::stream<asio::ip::tcp::socket>> _wsStream;
    beast::flat_buffer _readBuf;
};

#endif

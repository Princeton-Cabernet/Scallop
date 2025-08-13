
#ifndef WEB_SOCKET_SESSION_H
#define WEB_SOCKET_SESSION_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <optional>
#include <iostream>

using namespace boost;

template <typename Meta>
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession<Meta>> {

    template <typename SessionType>
    friend class WebSocketServer;

public:
    typedef std::function<void (WebSocketSession&, const char*, std::size_t)> OnMessageHandler;
    typedef std::function<void (WebSocketSession&)> OnOpenHandler;
    typedef std::function<void (WebSocketSession&)> OnCloseHandler;

    explicit WebSocketSession(asio::ip::tcp::socket socket)
        : _remote(socket.remote_endpoint()),
          _wsStream(std::move(socket)) {

        _wsStream.accept();
    }

    void start() {
        _read();
    }

    void write(const std::string& data) {

        _wsStream.async_write(asio::buffer(data, data.length()),
            [](system::error_code ec, std::size_t len) {
            if (ec) {
                throw std::runtime_error("WebSocketClient: write() failed: " + ec.message());
            }
        });
    }

    [[nodiscard]] const asio::ip::tcp::endpoint& remote() const {
        return _remote;
    }

    Meta& meta() {
        return _meta;
    }

    ~WebSocketSession() = default;

private:

    void _read() {

        _wsStream.async_read(_readBuf, [this, self = this->shared_from_this()]
            (system::error_code ec, std::size_t len) {

            if (!ec) {

                if (_onMessage) {
                    (*_onMessage)(*this, static_cast<const char*>(_readBuf.data().data()), len);
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
                    throw std::runtime_error("WebSocketSession: _read() failed: "
                                             + std::to_string(ec.value()) + ": " + ec.message());
                }
            }
        });
    }

    Meta _meta;
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
    std::optional<OnCloseHandler> _onClose = std::nullopt;
    asio::ip::tcp::endpoint _remote;
    beast::websocket::stream<asio::ip::tcp::socket> _wsStream;
    beast::flat_buffer _readBuf;
};

#endif

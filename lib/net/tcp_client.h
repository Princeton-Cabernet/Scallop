
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <boost/asio.hpp>
#include <optional>

using namespace boost;

class TCPClient {

public:
    typedef std::function<void (TCPClient&)> OnOpenHandler;
    typedef std::function<void (TCPClient&, char*, std::size_t)> OnMessageHandler;
    typedef std::function<void (TCPClient&)> OnCloseHandler;

    explicit TCPClient(asio::io_context& io)
        : _io(io), _socket(_io) { }

    void onOpen(OnOpenHandler&& f) {
        _onOpen = std::move(f);
    }

    void onMessage(OnMessageHandler&& f) {
        _onMessage = std::move(f);
    }

    void onClose(OnCloseHandler&& f) {
        _onClose = std::move(f);
    }

    void connect(const std::string& ip, unsigned short port) {

        if (!_onMessage) {
            throw std::logic_error("TCPClient: connect(): no message handler set");
        }

        asio::ip::tcp::resolver resolver(_io);
        auto endpoints = resolver.resolve(ip, std::to_string(port));

        asio::async_connect(_socket, endpoints,
            [this](system::error_code ec, const asio::ip::tcp::endpoint& e) {

                if (!ec) {

                    if (_onOpen) {
                        (*_onOpen)(*this);
                    }

                    _read();
                } else {
                    throw std::runtime_error("TCPClient: connect() failed: " + std::to_string(ec.value()));
                }
        });
    }

    void write(const char* buf, std::size_t len) {

        asio::async_write(_socket, asio::buffer(buf, len),
            [](system::error_code ec, std::size_t len) {

            if (ec) {
                throw std::runtime_error("TCPClient: write() failed: " + std::to_string(ec.value()));
            }
        });
    }

private:

    void _read() {

        _socket.async_read_some(asio::buffer(_rxBuf, _RX_BUF_LEN),
            [this](system::error_code ec, std::size_t len) {

            if (!ec) {

                if (_onMessage) {
                    (*_onMessage)(*this, _rxBuf, len);
                }

                _read();

            } else {
                if (ec.value() == static_cast<int>(asio::error::eof)) {

                    if (_onClose) {
                        (*_onClose)(*this);
                    }

                } else {
                    throw std::runtime_error("TCPClient: _read() failed: " + std::to_string(ec.value()));
                }
            }
        });
    }

    asio::io_context& _io;
    static const std::size_t _RX_BUF_LEN = 1024;
    asio::ip::tcp::socket _socket;
    char _rxBuf[_RX_BUF_LEN] = {};

    std::optional<OnOpenHandler> _onOpen = std::nullopt;
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
    std::optional<OnCloseHandler> _onClose = std::nullopt;
};

#endif

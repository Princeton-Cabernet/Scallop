
#ifndef TCP_SESSION_H
#define TCP_SESSION_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <optional>
#include <iostream>

using namespace boost;

template <typename Meta>
class TCPSession : public std::enable_shared_from_this<TCPSession<Meta>> {

    template <typename SessionType>
    friend class TCPServer;

public:
    typedef std::function<void (TCPSession&)> OnOpenHandler;
    typedef std::function<void (TCPSession&, char*, std::size_t)> OnMessageHandler;
    typedef std::function<void (TCPSession&)> OnCloseHandler;

    explicit TCPSession(asio::ip::tcp::socket socket)
        : _remote(socket.remote_endpoint()),
          _socket(std::move(socket)) { }

    void start() {
        _read();
    }

    [[nodiscard]] const asio::ip::tcp::endpoint& remote() const {
        return _remote;
    }

    Meta& meta() {
        return _meta;
    }

    void write(const char* buf, std::size_t len) {

        asio::async_write(_socket, asio::buffer(buf, len),
            [](system::error_code ec, std::size_t len) {
              if (ec) {
                  throw std::runtime_error("TCPServer: write() failed: " + std::to_string(ec.value()));
              }
        });
    }

    ~TCPSession() = default;

private:

    void _read() {
        _socket.async_read_some(asio::buffer(_readBuf, _READ_BUF_LEN),
            [this, self = this->shared_from_this()](system::error_code ec, std::size_t len) {

            if (!ec) {

                if (_onMessage) {
                    (*_onMessage)(*this, _readBuf, len);
                }

                _read();

            } else {

                if (ec.value() == static_cast<int>(asio::error::eof)) {
                    if (_onClose) {
                        (*_onClose)(*this);
                    }
                } else {
                    throw std::runtime_error("TCPSession: _read() failed: " + std::to_string(ec.value()));
                }
            }
        });
    }

    Meta _meta;
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
    std::optional<OnCloseHandler> _onClose = std::nullopt;
    static const std::size_t _READ_BUF_LEN = 1024;
    asio::ip::tcp::endpoint _remote;
    asio::ip::tcp::socket _socket;
    char _readBuf[_READ_BUF_LEN] = { };
};

#endif

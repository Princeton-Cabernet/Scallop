
#ifndef WEB_SOCKET_CLIENT_H
#define WEB_SOCKET_CLIENT_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <optional>

using namespace boost;

class WebSocketClient {

public:

    typedef std::function<void (WebSocketClient&)> OnOpenHandler;
    typedef std::function<void (WebSocketClient&, char*, std::size_t)> OnMessageHandler;
    typedef std::function<void (WebSocketClient&)> OnCloseHandler;

    explicit WebSocketClient(asio::io_context& io)
        : _io(io), _ws(asio::make_strand(_io)) { }

    void onOpen(OnOpenHandler&& f) {
        _onOpen = std::move(f);
    }

    void onMessage(OnMessageHandler&& f) {
        _onMessage = std::move(f);
    }

    void onClose(OnCloseHandler&& f) {
        _onClose = std::move(f);
    }

    void connect(const std::string& ip, unsigned short port, const std::string& path = "/") {

        asio::ip::tcp::resolver resolver(_io);
        auto endpoints = resolver.resolve(ip, std::to_string(port));

        _remote = beast::get_lowest_layer(_ws).connect(endpoints);

        // disable timeout on tcp stream as ws has own timeout logic
        beast::get_lowest_layer(_ws).expires_never();

        // set suggested timeout settings for the ws
        _ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

        _ws.handshake(ip, path);

        if (_onOpen) {
            (*_onOpen)(*this);
        }

        _read();
    }

    void write(const char* buf, std::size_t len) {

        _ws.async_write(asio::buffer(buf, len), [](system::error_code ec, std::size_t len) {
            if (ec) {
                throw std::runtime_error("WebSocketClient: write() failed: "
                                         + std::to_string(ec.value()));
            }
        });
    }

    inline void write(const std::string& s) {

        write(s.c_str(), s.length());
    }

    bool isConnected() const {

        return _ws.is_open();
    }

    void close() {

        _ws.close(beast::websocket::close_code::normal);
    }

    [[nodiscard]] asio::ip::tcp::endpoint remote() const {

        return _remote;
    }

private:

    void _read() {

        _ws.async_read_some(asio::buffer(_rxBuf, _RX_BUF_LEN),
            [this](system::error_code ec, std::size_t len) {

            if (!ec) {

                if (_onMessage) {
                    (*_onMessage)(*this, _rxBuf, len);
                }

                _read();
            }

        });
    }

    asio::io_context& _io;
    beast::websocket::stream<beast::tcp_stream> _ws;
    asio::ip::tcp::endpoint _remote = {};

    static const std::size_t _RX_BUF_LEN = 4096;
    char _rxBuf[_RX_BUF_LEN] = {};

    std::optional<OnOpenHandler> _onOpen = std::nullopt;
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
    std::optional<OnCloseHandler> _onClose = std::nullopt;
};

#endif
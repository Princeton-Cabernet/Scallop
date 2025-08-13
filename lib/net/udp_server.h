
#ifndef P4SFU_UDP_SERVER_H
#define P4SFU_UDP_SERVER_H

#include <boost/asio.hpp>
#include <optional>
#include <functional>
#include <iostream>

using namespace boost;

class UDPInterface {
public:

    typedef std::function<void(UDPInterface&, asio::ip::udp::endpoint&, const char*, std::size_t)>
        OnMessageHandler;

    virtual void sendTo(const asio::ip::udp::endpoint& to, const char* buf, std::size_t len) = 0;

    void onMessage(OnMessageHandler&& f) {
        _onMessage = std::move(f);
    }

    virtual ~UDPInterface() = default;

protected:
    std::optional<OnMessageHandler> _onMessage = std::nullopt;
};

namespace test {
    class MockUDPServer : public UDPInterface {
    public:

        struct Pkt {
            asio::ip::udp::endpoint to;
            std::array<char, 2048> buf;
            std::size_t len;
        };

        std::function<void (const Pkt&)> sentPacketHandler;

        void receivePacket(asio::ip::udp::endpoint& from, char* buf, std::size_t len) {
            (*_onMessage)(*this, from, buf, len);
        }

        void sendTo(const asio::ip::udp::endpoint& to, const char* buf, std::size_t len) override {
            Pkt pkt;
            pkt.to = to;
            pkt.len = len;
            std::memcpy(pkt.buf.data(), buf, len);
            sentPacketHandler(pkt);
        }
    };
}

class UDPServer : public UDPInterface {

public:

    UDPServer(asio::io_context& io, unsigned short port)
        : _socket(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)) {

        _read();
    }

    virtual void sendTo(const asio::ip::udp::endpoint& to, const char* buf, std::size_t len) override {

        _socket.async_send_to(asio::buffer(buf, len), to,
            [](system::error_code ec, std::size_t len) {

            if (ec) {
                throw std::runtime_error("UDPServer: sendTo() failed: " + std::to_string(ec.value()));
            }
        });
    }

private:

    void _read() {

        _socket.async_receive_from(asio::buffer(_rxBuf, _RX_BUF_LEN), _senderEndpoint,
            [this](system::error_code ec, std::size_t len) {

            if (!ec && len > 0) {

                if (_onMessage) {
                    (*_onMessage)(*this, _senderEndpoint, _rxBuf, len);
                }

                _read();
            } else {

                if (ec && ec.value() != static_cast<int>(asio::error::eof)) {
                    throw std::runtime_error("UDPServer: _read() failed: "
                        + std::to_string(ec.value()));
                }
            }
        });
    }

    static const std::size_t _RX_BUF_LEN = 2048;
    asio::ip::udp::socket _socket;
    asio::ip::udp::endpoint _senderEndpoint;
    char _rxBuf[_RX_BUF_LEN] = {};
};

#endif

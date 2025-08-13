
#include "api_client.h"

p4sfu::APIClient::APIClient(boost::asio::io_context& io)
    : _io{io}, _ws{_io} {

    _ws.onOpen([](WebSocketClient& c) { });

    _ws.onMessage([this](WebSocketClient& c, char* buf, std::size_t len) {
        this->_onMessage(c, buf, len);
    });

    _ws.onClose([](WebSocketClient& c) { });
}

void p4sfu::APIClient::open(const std::string& host, unsigned short port) {
    _ws.connect(host, port);
}

bool p4sfu::APIClient::isOpen() const {
    return _ws.isConnected();
}

void p4sfu::APIClient::close() {
    _ws.close();
}

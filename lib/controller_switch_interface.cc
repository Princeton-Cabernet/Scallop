
#include "controller_switch_interface.h"
#include "util.h"
#include "log.h"

const std::string p4sfu::ControllerSwitchInterface::CLASS = "ControllerSwitchInterface";

p4sfu::ControllerSwitchInterface::ControllerSwitchInterface(asio::io_context& io,
   unsigned short port)
   : _server(io, port) {

    _server.onOpen([this](auto&&... args) {
        _onSwitchOpen(std::forward<decltype(args)>(args)...);
    });

    _server.onMessage([this](auto&&... args) {
        _onSwitchMessage(std::forward<decltype(args)>(args)...);
    });

    _server.onClose([this](auto&&... args) {
        _onSwitchClose(std::forward<decltype(args)>(args)...);
    });
}

unsigned p4sfu::ControllerSwitchInterface::countRegistered() {

    return std::count_if(_connections.begin(), _connections.end(),
        [](auto &c) -> bool { return c.second.meta().registered; });
}

bool p4sfu::ControllerSwitchInterface::exists(unsigned switchId) {

    return _connectionById(switchId, true).has_value();
}

p4sfu::ControllerSwitchConnection& p4sfu::ControllerSwitchInterface::get(unsigned switchId) {

    auto conn = _connectionById(switchId);

    if (!conn) {
        throw std::logic_error(CLASS + ": get: switch does not exist, id="
            + std::to_string(switchId));
    }

    return (*conn)->second;
}

p4sfu::ControllerSwitchConnection& p4sfu::ControllerSwitchInterface::operator[](unsigned switchId) {

    return get(switchId);
}

void p4sfu::ControllerSwitchInterface::remove(unsigned switchId) {

    auto conn = _connectionById(switchId);

    if (!conn) {
        throw std::logic_error(
            CLASS + ": remove: switch does not exist, id=" + std::to_string(switchId));
    }

    _connections.erase(*conn);
}

void p4sfu::ControllerSwitchInterface::_onSwitchOpen(WebSocketSession<SwitchMeta>& s) {

    s.start();

    auto [it, success] = _connections.emplace(s.remote(), ControllerSwitchConnection{s});

    if (success) {
        onOpen(it->second);

        Log(Log::DEBUG) << CLASS << ": _onSwitchOpen: added connection" << std::endl;

    } else {
        Log(Log::ERROR) << CLASS << ": _onSwitchOpen: failed adding new connection" << std::endl;
    }
}

void p4sfu::ControllerSwitchInterface::_onSwitchMessage(WebSocketSession<SwitchMeta>& s,
    const char* buf, std::size_t len) {

    Log(Log::DEBUG) << CLASS << ": _onSwitchMessage: id=" << s.meta().id << ", msg="
                    << util::bufferString(buf, len) << std::endl;

    auto msg = _parseSwitchMessage(buf, len);

    if (msg) {

        if (s.meta().registered) {
            if ((*msg)["type"] == rpc::sw::Hello::TYPE_IDENT) {
                _onSwitchHello(s, rpc::sw::Hello{*msg});
            }
        } else { // only allow hello message from non-registered clients
            if ((*msg)["type"] == rpc::sw::Hello::TYPE_IDENT) {
                _onSwitchHello(s, rpc::sw::Hello{*msg});
            } else {
                Log(Log::ERROR) << CLASS << ": _onSwitchMessage: msg="
                                << util::bufferString(buf, len)
                                << ": non-hello message from not-registered client" << std::endl;
            }
        }
    }
}

void p4sfu::ControllerSwitchInterface::_onSwitchHello(WebSocketSession<SwitchMeta>& s,
                                                      const rpc::sw::Hello& m) {

    Log(Log::DEBUG) << CLASS << ": _onSwitchHello" << std::endl;

    auto conn_it = _connections.find(s.remote());

    if (conn_it == _connections.end()) {
        Log(Log::ERROR) << CLASS << ": _onSwitchHello: hello from unregistered switch, aborting ..."
                        << std::endl;
        return;
    }

    if (!s.meta().registered) {

        if (m.switchId.has_value()) {
            try {
                onNewSwitch(conn_it->second, m);
            } catch (std::bad_function_call &e) {
                Log(Log::ERROR) << CLASS << ": _onSwitchHello: no onNewSwitch event "
                                << "handler set, aborting ..." << std::endl;
            }

        } else {
            Log(Log::ERROR) << CLASS << ": _onSwitchHello: hello from unregistered client without "
                            << "id, aborting ..." << std::endl;
        }
    } else {
        try {
            onHello(conn_it->second, m);
        } catch (std::bad_function_call& e) {
            Log(Log::ERROR) << CLASS << ": _onSwitchHello: no onHello event handler set, "
                            << "aborting ..." << std::endl;
        }
    }
}

void p4sfu::ControllerSwitchInterface::_onSwitchClose(WebSocketSession<SwitchMeta>& s) {

    try {
        onClose(s.meta());
    } catch (std::bad_function_call &e) {
        Log(Log::ERROR) << CLASS << ": _onSwitchClose: no onClose event handler set" << std::endl;
    }
}

std::optional<json::json> p4sfu::ControllerSwitchInterface::_parseSwitchMessage(const char* buf,
    std::size_t len) {

    try {
        json::json msg = json::json::parse(buf, buf + len);

        if (msg.find("type") != msg.end()
            && (msg["type"] == "hello")) {
            return msg;
        } else {
            throw std::runtime_error("invalid message type");
        }

    } catch (std::exception& e) {
        Log(Log::ERROR)
                << CLASS << ": _parseSwitchMessage: failed parsing message: what=" << e.what()
                << ", msg=" << util::hexString(buf, len) << std::endl;
    }

    return std::nullopt;
}

std::optional<std::map<asio::ip::tcp::endpoint, p4sfu::ControllerSwitchConnection>::iterator>
    p4sfu::ControllerSwitchInterface::_connectionById(unsigned switchId, bool reg) {

    auto conn_it = std::find_if(_connections.begin(), _connections.end(), [switchId, reg](auto &c) {
        return !reg ? c.second.meta().id == switchId
            : c.second.meta().id == switchId && c.second.meta().registered;
    });

    return conn_it != _connections.end() ? std::make_optional<decltype(conn_it)>(conn_it)
        : std::nullopt;
}

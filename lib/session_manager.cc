
#include "session_manager.h"

bool p4sfu::SessionManager::exists(unsigned id) {

    return _sessions.find(id) != _sessions.end();
}

p4sfu::Session& p4sfu::SessionManager::add(unsigned id, const SFUConfig& sfuConfig) {

    auto [it, insertSuccess] = _sessions.insert({id, Session{id, sfuConfig}});

    if (insertSuccess) {
        auto&& [_, session] = *it;
        session.onNewStream([this](const Session& ss, unsigned p, const Stream& st) {
            this->_onNewStream(ss, p, st); // pass through to controller
        });
        return session;
    } else {
        throw std::logic_error("SessionManager: SessionManager: add: session with id " +
                               std::to_string(id) + " already exists.");
    }
}

p4sfu::Session& p4sfu::SessionManager::get(unsigned id) {

    if (!exists(id)) {
        throw std::logic_error("SessionManager: get: session with id " + std::to_string(id)
                               + " does not exist.");
    }

    return _sessions.at(id);
}

p4sfu::Session& p4sfu::SessionManager::operator[](unsigned id) {

        return get(id);
}

void p4sfu::SessionManager::remove(unsigned id) {

    if (!exists(id)) {
        throw std::logic_error("SessionManager: remove: session with id " + std::to_string(id)
                               + " does not exist.");
    }

    _sessions.erase(id);
}

const std::map<unsigned, p4sfu::Session>& p4sfu::SessionManager::sessions() const {

    return _sessions;
}

void  p4sfu::SessionManager::onNewStream(std::function<void
    (const Session&, unsigned participantId, const Stream&)>&& handler) {

    _onNewStream = std::move(handler);
}
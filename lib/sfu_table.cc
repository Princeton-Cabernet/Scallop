
#include "sfu_table.h"

p4sfu::SFUTable::Match::Match(const net::IPv4Port& ipPort, p4sfu::SSRC ssrc)
    : _ipPort(ipPort), _ssrc(ssrc) { }

std::size_t p4sfu::SFUTable::Match::Hash::operator()(const Match &m) const noexcept {

    std::size_t h = 0;
    h ^= std::hash<net::IPv4Port>{}(m._ipPort) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= m._ssrc + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}

bool p4sfu::SFUTable::Match::Equal::operator()(const Match &a, const Match &b) const noexcept {

    return a._ipPort == b._ipPort && a._ssrc == b._ssrc;
}

net::IPv4Port p4sfu::SFUTable::Match::ipPort() const {

    return _ipPort;
}

std::uint32_t p4sfu::SFUTable::Match::ssrc() const {

    return _ssrc;
}

p4sfu::SFUTable::Action::Action(const net::IPv4Port& to)
    : _to(to) { }

net::IPv4Port p4sfu::SFUTable::Action::to() const {

    return _to;
}

bool p4sfu::SFUTable::Action::operator==(const p4sfu::SFUTable::Action& other) const {

    return _to == other._to;
}

std::list<p4sfu::SFUTable::Action>& p4sfu::SFUTable::Entry::actions() {

    return _actions;
}

void p4sfu::SFUTable::Entry::addAction(const Action& action) {

    if (hasAction(action)) {
        throw std::invalid_argument("SFUTable::Entry: addAction: Action already exists");
    }

    _actions.push_back(action);
}

bool p4sfu::SFUTable::Entry::hasAction(const Action& action) const {
    return std::find(_actions.begin(), _actions.end(), action) != _actions.end();
}

unsigned long p4sfu::SFUTable::size() const {
    return _table.size();
}

bool p4sfu::SFUTable::hasMatch(const Match& m) const {

    return _table.find(m) != _table.end();
}

p4sfu::SFUTable::Entry& p4sfu::SFUTable::addMatch(const Match& m) {

    if (hasMatch(m)) {
        throw std::invalid_argument("SFUTable: addMatch: Match already exists");
    }

    auto insertResult = _table.insert(std::make_pair(m, Entry{}));

    if (!insertResult.second) {
        throw std::runtime_error("SFUTable: addMatch: failed adding match");
    }

    return insertResult.first->second;
}

p4sfu::SFUTable::Entry& p4sfu::SFUTable::getEntry(const Match& m) {

    if (!hasMatch(m)) {
        throw std::invalid_argument("SFUTable: getMatch: match does not exist");
    }

    return _table.find(m)->second;
}

p4sfu::SFUTable::Entry& p4sfu::SFUTable::operator[](const Match& m) {

    if (!hasMatch(m)) {
        throw std::invalid_argument("SFUTable: getMatch: match does not exist");
    }

    return _table.find(m)->second;
}


#ifndef P4SFU_SFU_TABLE_H
#define P4SFU_SFU_TABLE_H

#include <list>
#include <unordered_map>

#include "net/net.h"
#include "av1.h"
#include "p4sfu.h"
#include "sequence_rewriter.h"
#include "drop_layer_set.h"

namespace p4sfu {

    class SFUTable {

    public:
        class Match {

        public:

            explicit Match(const net::IPv4Port& ip, p4sfu::SSRC ssrc);

            struct Hash {
                std::size_t operator()(const Match& m) const noexcept;
            };

            struct Equal {
                bool operator()(const Match& a, const Match& b) const noexcept;
            };

            [[nodiscard]] net::IPv4Port ipPort() const;
            [[nodiscard]] p4sfu::SSRC ssrc() const;

        private:
            net::IPv4Port _ipPort   = {};
            p4sfu::SSRC _ssrc = 0;
        };

        class Action {

        public:
            explicit Action(const net::IPv4Port& to);
            [[nodiscard]] net::IPv4Port to() const;
            bool operator==(const Action& other) const;
            std::optional<av1::svc::L1T3> svcConfig = std::nullopt;
            SequenceRewriter sequenceRewriter;

        private:
            net::IPv4Port _to = {};
        };

        class Entry {

        public:
            Entry() = default;
            [[nodiscard]] std::list<Action>& actions();
            void addAction(const Action& action);
            [[nodiscard]] bool hasAction(const Action& action) const;

        private:
            std::list<Action> _actions;
        };

        SFUTable() = default;
        SFUTable(const SFUTable&) = default;
        SFUTable& operator=(const SFUTable&) = default;

        [[nodiscard]] unsigned long size() const;
        [[nodiscard]] bool hasMatch(const Match& m) const;
        Entry& addMatch(const Match& m);
        [[nodiscard]] Entry& getEntry(const Match& m);
        [[nodiscard]] Entry& operator[](const Match& m);

    private:
        std::unordered_map<Match, Entry, Match::Hash, Match::Equal> _table;
    };
}

#endif

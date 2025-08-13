
#ifndef P4_SFU_SESSION_MANAGER_H
#define P4_SFU_SESSION_MANAGER_H

#include <unordered_map>

#include "session.h"
#include "sfu_config.h"

namespace p4sfu {

    class Session;

    class SessionManager {
    public:
        bool exists(unsigned id);
        Session& add(unsigned id, const SFUConfig& sfuConfig);
        Session& get(unsigned id);
        Session& operator[](unsigned id);
        void remove(unsigned id);

        [[nodiscard]] const std::map<unsigned, Session>& sessions() const;

        void onNewStream(std::function<void
                (const Session&, unsigned participantId,const Stream&)>&& onNewStream);

    private:
        std::map<unsigned, Session> _sessions;
        std::function<void (const Session&, unsigned particpantId, const Stream&)> _onNewStream;
    };
}

#endif
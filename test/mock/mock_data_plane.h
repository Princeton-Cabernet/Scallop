
#include "data_plane.h"

namespace test {

    class MockDataPlane : public p4sfu::DataPlane {

    public:
        void sendPacket(const PktOut&) { }
        void addStream(const Stream& s) { };
        void removeStream(const Stream& s) { };
    };
}

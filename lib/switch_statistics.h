
#ifndef P4SFU_SWITCH_STATISTICS_H
#define P4SFU_SWITCH_STATISTICS_H

#include <iostream>
#include <unordered_map>

namespace p4sfu {

    struct TotalPacketStatistics {
        unsigned long pkts                   = 0;
        unsigned long bytes                  = 0;
        unsigned long stunPkts               = 0;
        unsigned long rtcpPkts               = 0;
        unsigned long rtpPkts                = 0;
        unsigned long frames                 = 0;
        unsigned long av1SimpleDescriptors   = 0;
        unsigned long av1ExtendedDescriptors = 0;
    };
}

#endif

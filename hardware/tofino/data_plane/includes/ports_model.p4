#pragma once

#if __TARGET_TOFINO__ == 1
    #define CPU_PORT 64
    #define CABERNET802_PORT 4
    #define CABMEATY02_PORT 16
#elif __TARGET_TOFINO__ == 2
    #define CPU_PORT 0
    #define CABERNET802_PORT 4
    #define CABMEATY02_PORT 16
#endif
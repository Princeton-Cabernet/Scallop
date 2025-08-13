
#include <catch.h>
#include "proto/stun.h"
#include <util.h>

#include "stun_packets.h"

#include <iostream>
#include <iomanip>
#include <cstring>

TEST_CASE("stun: bufferContainsSTUN", "[stun]") {

    CHECK(stun::bufferContainsSTUN((const unsigned char*) test::stun_bind_req,
                                   test::stun_bind_req_len));
}

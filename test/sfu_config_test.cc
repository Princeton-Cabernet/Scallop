#include <catch.h>

#include <sfu_config.h>

using namespace p4sfu;

TEST_CASE("SFUConfig: ()", "[sfuconfig]") {
    
    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};

    CHECK(c.ip() == "1.2.3.4");
    CHECK(c.port() == 3482);
    CHECK(c.iceUfrag() == "7Ad/");
    CHECK(c.icePwd() == "UKZe/aYNEouGzQUhChnKGiIS");
    CHECK(c.netLimit().ip() == net::IPv4{"0.0.0.0"});
    CHECK(c.netLimit().prefix() == 0);

    CHECK(c.sessionTemplate().origin.unicastAddr == "1.2.3.4");
    CHECK(c.iceCandidate().address == "1.2.3.4");
    CHECK(c.iceCandidate().port == 3482);
}

#include <catch.h>

#include <net/net.h>


TEST_CASE("IPv4", "[net]") {

    SECTION("parses an IPv4 address") {

        net::IPv4 a, b, c;

        CHECK_NOTHROW(a = net::IPv4("12.2.23.42"));
        CHECK(a.num() == 0x0c02172a);
        CHECK(a.str() == "12.2.23.42");

        CHECK_NOTHROW(b = net::IPv4("255.255.255.255"));
        CHECK(b.num() == 0xffffffff);
        CHECK(b.str() == "255.255.255.255");

        CHECK_NOTHROW(c = net::IPv4("0.0.0.0"));
        CHECK(c.num() == 0x00000000);
        CHECK(c.str() == "0.0.0.0");
    }

    SECTION("throws an exception when provided an invalid address") {
        CHECK_THROWS(net::IPv4("482.492"));
        CHECK_THROWS_AS(net::IPv4(""), std::invalid_argument);
        CHECK_THROWS_AS(net::IPv4("256.22.42.5"), std::invalid_argument);
        CHECK_THROWS_AS(net::IPv4("23.43.2.100.23"), std::invalid_argument);
    }
}

TEST_CASE("IPv4::isInSubnet", "[net]") {

    net::IPv4 a{"10.0.2.3"};
    CHECK(a.isInSubnet(net::IPv4{"10.0.2.0"}, 24));
    CHECK_FALSE(a.isInSubnet(net::IPv4{"10.0.0.0"}, 24));
    CHECK(a.isInSubnet(net::IPv4{"10.0.0.0"}, 16));
    CHECK(a.isInSubnet(net::IPv4{"0.0.0.0"}, 0));
}

TEST_CASE("IPv4Port", "[net]") {

    net::IPv4Port ep{net::IPv4{"12.12.12.12"}, 2392};

    SECTION("can be initialized using a ip/port combination") {
        CHECK(ep.ip() == net::IPv4{"12.12.12.12"});
        CHECK(ep.port() == 2392);
    }

    SECTION("is compatible with std::hash") {
        CHECK(std::hash<net::IPv4Port>{}(ep) == 0xc0c0c0c0958);
    }
}

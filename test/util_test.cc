
#include <catch.h>
#include <util.h>

TEST_CASE("util: crc32", "[util]") {

    // https://crccalc.com

    unsigned char test1[] = "test";
    unsigned char test2[] = "p4sfu";
    unsigned char test3[] = "t43rxdqewcrvwe5byb5ret4weceedvcfd";

    CHECK(util::crc32(0, test1, 4) == 0xd87f7e0c);
    CHECK(util::crc32(0, test2, 5) == 0x05C6E7BF);
    CHECK(util::crc32(0, test3, 33) == 0x0D5A72A6);
}

TEST_CASE("util: parseIPPort", "[util]") {

    auto [ip1, port1] = util::parseIPPort("127.0.0.1:3200");
    CHECK(ip1 == "127.0.0.1");
    CHECK(port1 == 3200);

    auto [ip2, port2] = util::parseIPPort("242.3.29.92:80");
    CHECK(ip2 == "242.3.29.92");
    CHECK(port2 == 80);

    CHECK_THROWS(util::parseIPPort(""));
    CHECK_THROWS(util::parseIPPort("42:23"));
    CHECK_THROWS(util::parseIPPort("242.3.29.92"));
    CHECK_THROWS(util::parseIPPort(":8022"));
    CHECK_THROWS(util::parseIPPort(":"));
    CHECK_THROWS(util::parseIPPort("42.23:23"));
    CHECK_THROWS(util::parseIPPort("abc:29"));
}

TEST_CASE("util: parseCIDR", "[util]") {

    auto [ip1, mask1] = util::parseCIDR("10.0.0.0/24");
    CHECK(ip1 == "10.0.0.0");
    CHECK(mask1 == 24);

    auto [ip2, mask2] = util::parseCIDR("0.0.0.0/0");
    CHECK(ip2 == "0.0.0.0");
    CHECK(mask2 == 0);

    CHECK_THROWS(util::parseCIDR("20.20.0.0.0/24"));
    CHECK_THROWS(util::parseCIDR("20.20.0.0.0/"));
    CHECK_THROWS(util::parseCIDR("20.20.0.0.0/d"));
}

TEST_CASE("util: splitString", "[util]") {

    SECTION("simple delimiter") {

        auto parts = util::splitString("1 2 3 4 5", " ");

        CHECK(parts.size() == 5);
        CHECK(parts[0] == "1");
        CHECK(parts[1] == "2");
        CHECK(parts[2] == "3");
        CHECK(parts[3] == "4");
        CHECK(parts[4] == "5");
    }

    SECTION("multi-char delimiter") {

        auto parts = util::splitString("fsdufi-jfksdljfl--sjdkf", "--");
        CHECK(parts.size() == 2);
        CHECK(parts[0] == "fsdufi-jfksdljfl");
        CHECK(parts[1] == "sjdkf");
    }

    SECTION("escape chars") {

        auto parts = util::splitString("fsdufi\n\rjfksdljfl\n\rsjdkf", "\n\r");
        CHECK(parts.size() == 3);
        CHECK(parts[0] == "fsdufi");
        CHECK(parts[1] == "jfksdljfl");
        CHECK(parts[2] == "sjdkf");
    }
}

TEST_CASE("util: formatString", "[util]") {

    CHECK(util::formatString("%u %s", 5, "hello") == "5 hello");
    CHECK(util::formatString("num: %i", 22323) == "num: 22323");
    CHECK(util::formatString("%s%s", "hello", "hello") == "hellohello");
    CHECK(util::formatString("%u%%", 10) == "10%");
}

TEST_CASE("util: joinStrings", "[util]") {

    CHECK(util::joinStrings(std::vector<std::string>{"1", "2", "3"}, " - ") == "1 - 2 - 3");
    CHECK(util::joinStrings(std::vector<std::string>{"1", "2", "3"}) == "1 2 3");
    CHECK(util::joinStrings(std::vector<std::string>{"fjdsk", "23290"}, "*/") == "fjdsk*/23290");
}

TEST_CASE("util: extractBits", "[util]") {

    // 10'1011'1011, len = 10
    std::vector<bool> v1 = { true, false, true, false, true, true, true, false, true, true };

    SECTION("throws when given an invalid range") {

//        CHECK_THROWS_AS(util::extractBits(v1, 0, 14), std::invalid_argument);
//        CHECK_THROWS_AS(util::extractBits(v1, 8, 4), std::invalid_argument);
//        CHECK_THROWS_AS(util::extractBits(v1, 9, 2), std::invalid_argument);
//        CHECK_THROWS_AS(util::extractBits(v1, 0, 0), std::invalid_argument);
//        CHECK_THROWS_AS(util::extractBits(v1, 0, 33), std::invalid_argument);
        CHECK_NOTHROW(util::extractBits(v1, 9, 1));
        CHECK_NOTHROW(util::extractBits(v1, 8, 2));
    }

    SECTION("extracts a bit range") {

        CHECK(util::extractBits(v1, 0) == 0b1);
        CHECK(util::extractBits(v1, 0, 10) == 0b10'1011'1011);
        CHECK(util::extractBits(v1, 4, 4) == 0b1110);

    }
}

TEST_CASE("util: extractNonSymmetric", "[util]") {

    std::vector<bool> v1 = { false, false };
    std::vector<bool> v2 = { false, true };
    std::vector<bool> v3 = { true, false };
    std::vector<bool> v4 = { true, true, false };
    std::vector<bool> v5 = { true, true, true };

    CHECK(util::extractNonSymmetric(v1, 0, 5) == std::pair<std::uint32_t, std::uint32_t>{0b000, 2});
    CHECK(util::extractNonSymmetric(v2, 0, 5) == std::pair<std::uint32_t, std::uint32_t>{0b001, 2});
    CHECK(util::extractNonSymmetric(v3, 0, 5) == std::pair<std::uint32_t, std::uint32_t>{0b010, 2});
    CHECK(util::extractNonSymmetric(v4, 0, 5) == std::pair<std::uint32_t, std::uint32_t>{0b011, 3});
    CHECK(util::extractNonSymmetric(v5, 0, 5) == std::pair<std::uint32_t, std::uint32_t>{0b100, 3});
}

#include <catch.h>
#include <bitstream.h>

const unsigned char bits[] = {0x35, 0xca, 0x06, 0x3f};
Bitstream bs(bits, 4); // 0011 0101 1100 1010 0000 0110 0011 1111

TEST_CASE("Bitstream: bitCount", "[bitstream]") {

    CHECK(bs.bitCount() == 32);
}

TEST_CASE("Bitstream: operator[]", "[bitstream]") {

    CHECK(bs.bitCount() == 32);
    CHECK_FALSE(bs[0]);
    CHECK(bs[5]);
    CHECK(bs[12]);
    CHECK_FALSE(bs[19]);
    CHECK(bs[30]);
    CHECK_THROWS(bs[55]);
}

TEST_CASE("Bitstream: extract", "[bitstream]") {

    CHECK(bs.extract(0)  == 0);
    CHECK(bs.extract(5)  == 1);
    CHECK(bs.extract(12) == 1);
    CHECK(bs.extract(19) == 0);
    CHECK(bs.extract(30) == 1);
    CHECK_THROWS(bs.extract(55));

    CHECK_THROWS(bs.extract(32, 32));
    CHECK_THROWS(bs.extract(43, 4));
    CHECK_NOTHROW(bs.extract(30, 2));
    CHECK_THROWS(bs.extract(32, 1));
    CHECK_THROWS(bs.extract(31, 2));
    CHECK_NOTHROW(bs.extract(0, 32));
    CHECK_THROWS(bs.extract(0, 0));

    CHECK(bs.extract(0, 4)  == 0x03);
    CHECK(bs.extract(0, 32) == 0x35ca063f);
    CHECK(bs.extract(4, 4)  == 0x05);
    CHECK(bs.extract(0, 8)  == 0x35);
    CHECK(bs.extract(8, 8)  == 0xca);
    CHECK(bs.extract(16, 8) == 0x06);
    CHECK(bs.extract(24, 8) == 0x3f);
}

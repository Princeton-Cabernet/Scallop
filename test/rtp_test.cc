#include <catch.h>
#include <arpa/inet.h>

#include "proto/rtp.h"
#include "rtp_rtcp_packets.h"

TEST_CASE("rtp::hdr", "[rtp]") {

    auto* rtp1 = (rtp::hdr*) test::rtp_buf1;

    CHECK(rtp1->version() == 2);
    CHECK(rtp1->padding() == 0);
    CHECK(rtp1->extension() == 1);
    CHECK(rtp1->csrc_count() == 0);
    CHECK(rtp1->marker() == 1);
    CHECK(rtp1->payload_type() == 104);
    CHECK(ntohs(rtp1->seq) == 20223);
    CHECK(ntohl(rtp1->ts) == 2941283823);
    CHECK(ntohl(rtp1->ssrc) == 0x6a70d0e8);

    auto* rtp2 = (rtp::hdr*) test::rtp_buf2;

    CHECK(rtp2->version() == 2);
    CHECK(rtp2->padding() == 0);
    CHECK(rtp2->extension() == 1);
    CHECK(rtp2->csrc_count() == 0);
    CHECK(rtp2->marker() == 1);
    CHECK(rtp2->payload_type() == 45);
    CHECK(ntohs(rtp2->seq) == 41883);
    CHECK(ntohl(rtp2->ts) == 3004101408);
    CHECK(ntohl(rtp2->ssrc) == 0x80dff796);
}

TEST_CASE("rtp::extension_profile_header", "[rtp]") {

    SECTION("parses header for 1-byte extensions") {
        auto* h = reinterpret_cast<rtp::extension_profile_hdr*>(test::rtp_buf1 + rtp::HDR_LEN);
        CHECK(ntohs(h->profile) == 0xbede);
        CHECK(ntohs(h->profile) == (std::uint16_t) rtp::ext_profile::one_byte);
        CHECK(ntohs(h->len) == 4);
        CHECK(h->byte_len() == 16);
    }

    SECTION("parses header for 2-byte extensions") {
        auto* h = reinterpret_cast<rtp::extension_profile_hdr*>(test::rtp_buf3 + rtp::HDR_LEN);
        CHECK(ntohs(h->profile) == 0x1000);
        CHECK(ntohs(h->profile) == (std::uint16_t) rtp::ext_profile::two_byte);
        CHECK(ntohs(h->len) == 8);
        CHECK(h->byte_len() == 32);
    }
}

TEST_CASE("rtp::extension_profile", "[rtp]") {

    auto* rtp2 = reinterpret_cast<rtp::hdr *>(test::rtp_buf2);
    auto* rtp3 = reinterpret_cast<rtp::hdr *>(test::rtp_buf3);

    CHECK(rtp2->extension_profile() == rtp::ext_profile::one_byte);
    CHECK(rtp3->extension_profile() == rtp::ext_profile::two_byte);
}

TEST_CASE("rtp::ext_type", "[rtp]") {

    SECTION("extracts the extension type for 1-byte extensions") {
        unsigned char ext_buf1[] = { 0x22, 0x72, 0xec, 0x40 };
        unsigned char ext_buf2[] = { 0x90, 0x30 };
        unsigned char ext_buf3[] = { 0xd2, 0xc9, 0x74, 0xf6 };
        CHECK(rtp::ext_type(ext_buf1) == 2);
        CHECK(rtp::ext_type(ext_buf2) == 9);
        CHECK(rtp::ext_type(ext_buf3) == 13);
    }

    SECTION("extracts the extension type for 2-byte extensions") {
        unsigned char ext_buf4[] = { 0x02, 0x03, 0x6b, 0x10, 0x39 };
        unsigned char ext_buf5[] = { 0x09, 0x01, 0x30 };
        CHECK(rtp::ext_type(ext_buf4, rtp::ext_profile::two_byte) == 2);
        CHECK(rtp::ext_type(ext_buf5, rtp::ext_profile::two_byte) == 9);
    }
}

TEST_CASE("rtp::ext_len", "[rtp]") {

    SECTION("extracts the extension length for 1-byte extensions") {
        unsigned char ext_buf1[] = { 0x22, 0x72, 0xec, 0x40 };
        unsigned char ext_buf2[] = { 0x90, 0x30 };
        unsigned char ext_buf3[] = { 0xd2, 0xc9, 0x74, 0xf6 };
        CHECK(rtp::ext_len(ext_buf1) == 3);
        CHECK(rtp::ext_len(ext_buf2) == 1);
        CHECK(rtp::ext_len(ext_buf3) == 3);
    }

    SECTION("extracts the extension length for 2-byte extensions") {
        unsigned char ext_buf4[] = { 0x02, 0x03, 0x6b, 0x10, 0x39 };
        unsigned char ext_buf5[] = { 0x09, 0x01, 0x30 };
        CHECK(rtp::ext_len(ext_buf4, rtp::ext_profile::two_byte) == 3);
        CHECK(rtp::ext_len(ext_buf5, rtp::ext_profile::two_byte) == 1);
    }
}

TEST_CASE("rtp::hdr::extension_ptrs", "[rtp]") {

    SECTION("returns pointers to 1-byte extensions") {

        auto* rtp2 = reinterpret_cast<rtp::hdr *>(test::rtp_buf2);
        auto exts = rtp2->extension_ptrs();

        CHECK(exts.size() == 4);

        CHECK(exts[0] - test::rtp_buf2 == 16);
        CHECK(rtp::ext_type(exts[0]) == 2);
        CHECK(rtp::ext_len(exts[0]) == 3);

        CHECK(exts[1] - test::rtp_buf2 == 20);
        CHECK(rtp::ext_type(exts[1]) == 4);
        CHECK(rtp::ext_len(exts[1]) == 2);

        CHECK(exts[2] - test::rtp_buf2 == 23);
        CHECK(rtp::ext_type(exts[2]) == 9);
        CHECK(rtp::ext_len(exts[2]) == 1);

        CHECK(exts[3] - test::rtp_buf2 == 25);
        CHECK(rtp::ext_type(exts[3]) == 13);
        CHECK(rtp::ext_len(exts[3]) == 3);
    }

    SECTION("returns pointers to 2-byte extensions") {

        auto* rtp3 = reinterpret_cast<rtp::hdr *>(test::rtp_buf3);
        auto exts = rtp3->extension_ptrs();

        CHECK(exts.size() == 3);

        CHECK(rtp::ext_type(exts[0], rtp::ext_profile::two_byte) == 2);
        CHECK(rtp::ext_len(exts[0], rtp::ext_profile::two_byte) == 3);

        CHECK(rtp::ext_type(exts[1], rtp::ext_profile::two_byte) == 9);
        CHECK(rtp::ext_len(exts[1], rtp::ext_profile::two_byte) == 1);

        CHECK(rtp::ext_type(exts[2], rtp::ext_profile::two_byte) == 12);
        CHECK(rtp::ext_len(exts[2], rtp::ext_profile::two_byte) == 20);
    }
}

TEST_CASE("rtp::hdr::extension_ptr", "[rtp]") {

    auto* rtp2 = reinterpret_cast<rtp::hdr *>(test::rtp_buf2);
    auto* rtp3 = reinterpret_cast<rtp::hdr *>(test::rtp_buf3);

    SECTION("returns a pointer to the extension if an extension exists") {

        CHECK(*(rtp2->extension_ptr(2)) == 0x22);
        CHECK(*(rtp2->extension_ptr(4)) == 0x41);
        CHECK(*(rtp2->extension_ptr(9)) == 0x90);
        CHECK(*(rtp2->extension_ptr(13)) == 0xd2);

        CHECK(*(rtp3->extension_ptr(9)) == 0x09);
        CHECK(*(rtp3->extension_ptr(12)) == 0x0c);
    }

    SECTION("returns nullptr if an extension does not exist") {

        CHECK(rtp2->extension_ptr(1) == nullptr);
        CHECK(rtp2->extension_ptr(5) == nullptr);
        CHECK(rtp2->extension_ptr(15) == nullptr);

        CHECK(rtp3->extension_ptr(1) == nullptr);
        CHECK(rtp3->extension_ptr(7) == nullptr);
    }
}

TEST_CASE("rtp::hdr::extensions", "[rtp]") {

    auto* rtp2 = reinterpret_cast<rtp::hdr *>(test::rtp_buf2);
    auto* rtp3 = reinterpret_cast<rtp::hdr *>(test::rtp_buf3);

    SECTION("returns a vector with ext structs for all 1-byte extensions") {

        auto extensions = rtp2->extensions();
        CHECK(extensions.size() == 4);

        CHECK(extensions[0].type == 2);
        CHECK(extensions[0].len == 3);
        CHECK(extensions[0].data[0] == 0x72);
        CHECK(extensions[0].data[1] == 0xec);
        CHECK(extensions[0].data[2] == 0x40);

        CHECK(extensions[1].type == 4);
        CHECK(extensions[1].len == 2);
        CHECK(extensions[1].data[0] == 0x76);
        CHECK(extensions[1].data[1] == 0x21);

        CHECK(extensions[2].type == 9);
        CHECK(extensions[2].len == 1);
        CHECK(extensions[2].data[0] == 0x30);

        CHECK(extensions[3].type == 13);
        CHECK(extensions[3].len == 3);
        CHECK(extensions[3].data[0] == 0xc9);
        CHECK(extensions[3].data[1] == 0x74);
        CHECK(extensions[3].data[2] == 0xf6);
    }

    SECTION("returns a vector with ext structs for all 2-byte extensions") {

        auto extensions = rtp3->extensions();
        CHECK(extensions.size() == 3);

        CHECK(extensions[0].type == 2);
        CHECK(extensions[0].len == 3);
        CHECK(extensions[0].data[0] == 0x6b);
        CHECK(extensions[0].data[1] == 0x10);
        CHECK(extensions[0].data[2] == 0x39);

        CHECK(extensions[1].type == 9);
        CHECK(extensions[1].len == 1);
        CHECK(extensions[1].data[0] == 0x30);

        CHECK(extensions[2].type == 12);
        CHECK(extensions[2].len == 20);
        CHECK(extensions[2].data[0] == 0xc0);
        CHECK(extensions[2].data[5] == 0x14);
        CHECK(extensions[2].data[11] == 0x14);
        CHECK(extensions[2].data[19] == 0x67);
    }
}

TEST_CASE("rtp::hdr::extension") {

    auto* rtp2 = reinterpret_cast<rtp::hdr *>(test::rtp_buf2);
    auto* rtp3 = reinterpret_cast<rtp::hdr *>(test::rtp_buf3);


    SECTION("returns a single extension") {

        auto ext2 = rtp2->extension(2);
        CHECK(ext2);
        CHECK(ext2->type == 2);
        CHECK(ext2->len == 3);
        CHECK(ext2->data[0] == 0x72);
        CHECK(ext2->data[1] == 0xec);
        CHECK(ext2->data[2] == 0x40);

        auto ext13 = rtp2->extension(13);
        CHECK(ext13);
        CHECK(ext13->type == 13);
        CHECK(ext13->len == 3);
        CHECK(ext13->data[0] == 0xc9);
        CHECK(ext13->data[1] == 0x74);
        CHECK(ext13->data[2] == 0xf6);

        auto ext9 = rtp3->extension(9);
        CHECK(ext9);
        CHECK(ext9->type == 9);
        CHECK(ext9->len == 1);
        CHECK(ext9->data[0] == 0x30);

        auto ext12 = rtp3->extension(12);
        CHECK(ext12);
        CHECK(ext12->type == 12);
        CHECK(ext12->len == 20);
        CHECK(ext12->data[0] == 0xc0);
        CHECK(ext12->data[5] == 0x14);
        CHECK(ext12->data[11] == 0x14);
        CHECK(ext12->data[19] == 0x67);
    }

    SECTION("returns nullopt if extension does not exist") {

        CHECK_FALSE(rtp2->extension(7));
        CHECK_FALSE(rtp2->extension(15));
        CHECK_FALSE(rtp3->extension(1));
        CHECK_FALSE(rtp3->extension(7));
    }
}

TEST_CASE("rtp::contains_rtp_or_rtcp", "[rtp]") {
    
    CHECK(rtp::contains_rtp_or_rtcp(test::rtp_buf1, 28));
}

TEST_CASE("rtp::serialize_extensions", "[rtp]") {

    unsigned char buf[128] = { 0 };

    SECTION("scenario 1") {

        std::vector<rtp::ext> extensions = {
            rtp::ext{ .type = 4, .len  = 3, .data = {0x01, 0x02, 0x03}},
            rtp::ext{ .type = 5, .len = 5, .data = {0x01, 0x02, 0x03, 0x04, 0x05}}
        };

        serialize_extensions(extensions, buf, 128, rtp::ext_profile::one_byte);
        CHECK(serialize_extensions(extensions, buf, 128, rtp::ext_profile::one_byte) == 16);

        // extension profile header:
        CHECK(buf[0] == 0xbe);
        CHECK(buf[1] == 0xde);
        CHECK(buf[2] == 0x00);
        CHECK(buf[3] == 0x03); // 3 bytes: length in 4-Byte words excl. ext. profile header

        // first extension:
        CHECK(buf[4] == 0x42);
        CHECK(buf[5] == 0x01);
        CHECK(buf[6] == 0x02);
        CHECK(buf[7] == 0x03);

        // second extension:
        CHECK(buf[8] == 0x54);
        CHECK(buf[9] == 0x01);
        CHECK(buf[10] == 0x02);
        CHECK(buf[11] == 0x03);
        CHECK(buf[12] == 0x04);
        CHECK(buf[13] == 0x05);

        // padding:
        CHECK(buf[14] == 0x00);
        CHECK(buf[15] == 0x00);
    }

    SECTION("scenario 2") {

        std::vector<rtp::ext> extensions = {
            rtp::ext{ .type = 13, .len  = 4, .data = {0xc9, 0x74, 0xf6, 0x01}}
        };

        rtp::serialize_extensions(extensions, buf, 128, rtp::ext_profile::one_byte);
        CHECK(rtp::serialize_extensions(extensions, buf, 128, rtp::ext_profile::one_byte) == 12);

        // extension profile header:
        CHECK(buf[0] == 0xbe);
        CHECK(buf[1] == 0xde);
        CHECK(buf[2] == 0x00);
        CHECK(buf[3] == 0x02); // 2 bytes: length in 4-Byte words excl. ext. profile header

        CHECK(buf[4] == 0xd3); // type 13, len 3
        CHECK(buf[5] == 0xc9);
        CHECK(buf[6] == 0x74);
        CHECK(buf[7] == 0xf6);
        CHECK(buf[8] == 0x01); // added byte

        // 3 byte padding
        CHECK(buf[9] == 0x00);
        CHECK(buf[10] == 0x00);
        CHECK(buf[11] == 0x00);
    }
}

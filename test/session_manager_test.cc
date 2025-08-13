
#include <catch.h>

#include <session.h>
#include <session_manager.h>

using namespace p4sfu;

TEST_CASE("SessionManager: exists", "[session_manager]") {

    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};
    SessionManager m;

    CHECK_FALSE(m.exists(0));
    CHECK_NOTHROW(m.add(2, c));
    CHECK(m.exists(2));
}

TEST_CASE("SessionManager: add", "[session_manager]") {

    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};
    SessionManager m;

    CHECK_NOTHROW(m.add(0, c));
    CHECK_NOTHROW(m.add(2, c));
    CHECK_THROWS_AS(m.add(0, c), std::logic_error);
    CHECK(m.exists(0));
    CHECK(m.exists(2));
}

TEST_CASE("SessionManager: get", "[session_manager]") {

    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};
    SessionManager m;

    CHECK_NOTHROW(m.add(0, c));
    CHECK_NOTHROW(m.add(2, c));

    CHECK(m.get(0).id() == 0);
    CHECK(m.get(2).id() == 2);
    CHECK_THROWS_AS(m.get(1), std::logic_error);
}

TEST_CASE("SessionManager: operator[]", "[session_manager]") {

    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};
    SessionManager m;

    CHECK_NOTHROW(m.add(0, c));
    CHECK_NOTHROW(m.add(2, c));

    CHECK(m[0].id() == 0);
    CHECK(m[2].id() == 2);
    CHECK_THROWS_AS(m[1], std::logic_error);
}

TEST_CASE("SessionManager: remove", "[session_manager]") {

    SFUConfig c{"1.2.3.4", 3482, "7Ad/", "UKZe/aYNEouGzQUhChnKGiIS", net::IPv4Prefix{"0.0.0.0", 0}};
    SessionManager m;

    CHECK_NOTHROW(m.add(0, c));
    CHECK_NOTHROW(m.add(2, c));

    CHECK_NOTHROW(m.remove(0));
    CHECK_FALSE(m.exists(0));
    CHECK_THROWS_AS(m.remove(1), std::logic_error);
    CHECK_THROWS_AS(m.get(1), std::logic_error);
}

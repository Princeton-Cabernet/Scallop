#include <catch.h>

#include <sfu_table.h>

using namespace p4sfu;

TEST_CASE("SFUTable: ()", "[sfu_table]") {

    SFUTable t;
    CHECK(t.size() == 0);
}

TEST_CASE("SFUTable: hasMatch()", "[sfu_table]") {

    SFUTable t;
    CHECK_FALSE(t.hasMatch(SFUTable::Match{net::IPv4Port{"1.2.2.4", 1232}, 1929230}));
    t.addMatch(SFUTable::Match{net::IPv4Port{"1.0.0.23", 3433}, 23020322});
    CHECK(t.hasMatch(SFUTable::Match{net::IPv4Port{"1.0.0.23", 3433}, 23020322}));
}

TEST_CASE("SFUTable: addMatch()", "[sfu_table]") {

    SECTION("adds matches") {

        SFUTable t;
        CHECK(t.size() == 0);
        CHECK_NOTHROW(t.addMatch(SFUTable::Match{net::IPv4Port{"1.2.2.4", 23242}, 83264783}));
        CHECK(t.size() == 1);
        CHECK_NOTHROW(t.addMatch(SFUTable::Match{net::IPv4Port{"1.0.0.23", 34393}, 3473802}));
        CHECK(t.size() == 2);
    }

    SECTION("throws when adding an already existing match") {

        SFUTable t;
        CHECK(t.size() == 0);
        CHECK_NOTHROW(t.addMatch(SFUTable::Match{net::IPv4Port{"1.2.2.4", 2132}, 245321382}));
        CHECK(t.size() == 1);
        CHECK_THROWS(t.addMatch(SFUTable::Match{net::IPv4Port{"1.2.2.4", 2132}, 245321382}));
    }
}

TEST_CASE("SFUTable: getEntry(), []", "[sfu_table]") {

    SFUTable t;

    SFUTable::Match existingMatch{net::IPv4Port{"1.2.2.4", 23823}, 783927459};
    SFUTable::Match nonExistingMatch{net::IPv4Port{"1.0.0.4", 23821}, 93243902};

    CHECK_NOTHROW(t.addMatch(existingMatch));
    CHECK(t.size() == 1);

    SECTION("throws when match does not exist") {

        CHECK_THROWS(t.getEntry(nonExistingMatch));
        CHECK_THROWS(t[nonExistingMatch]);
    }

    SECTION("retrieves an entry") {
        auto& e1 = t.getEntry(existingMatch);
        CHECK(e1.actions().empty());

        auto& e2 = t[existingMatch];
        CHECK(e2.actions().empty());
    }
}

TEST_CASE("SFUTable: Entry: hasAction()", "[sfu_table]") {

    SFUTable t;

    SFUTable::Match match{net::IPv4Port{"1.2.2.4", 23823}, 783927459};
    SFUTable::Action existingAction{net::IPv4Port{"5.6.7.8", 23825}};
    SFUTable::Action nonExistingAction{net::IPv4Port{"8.7.6.5", 23825}};

    auto entry = t.addMatch(match);
    entry.addAction(existingAction);
    CHECK(entry.hasAction(existingAction));
    CHECK_FALSE(entry.hasAction(nonExistingAction));
}


TEST_CASE("SFUTable: Entry: addAction()", "[sfu_table]") {

    SFUTable::Match match{net::IPv4Port{"1.2.2.4", 23823}, 783927459};
    SFUTable::Action action{net::IPv4Port{"5.6.7.8", 23825}};

    SFUTable t;
    auto& m = t.addMatch(match);
    CHECK_NOTHROW(m.addAction(action));

    SECTION("adds an action") {
        CHECK(m.hasAction(action));
        CHECK(m.actions().size() == 1);
    }

    SECTION("correctly initializes an action") {

        auto& a = m.actions().front();
        CHECK(a.to() == action.to());
        CHECK(a.svcConfig == std::nullopt);
    }

    SECTION("throws when adding an action that already exists") {
        CHECK_THROWS(m.addAction(action));
    }
}

TEST_CASE("SFUTable: Action: set svcConfig", "[sfu_table]") {

    SFUTable::Match match{net::IPv4Port{"1.2.2.4", 23823}, 783927459};
    SFUTable::Action action{net::IPv4Port{"5.6.7.8", 23825}};

    SFUTable t;
    auto& m = t.addMatch(match);
    CHECK_NOTHROW(m.addAction(action));

    auto& a = m.actions().front();
    a.svcConfig = av1::svc::L1T3{};
    CHECK(a.svcConfig);
    CHECK(a.svcConfig->decodeTarget == av1::svc::L1T3::DecodeTarget::hi);
}

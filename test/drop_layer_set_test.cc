
#include <catch.h>

#include <drop_layer_set.h>

TEST_CASE("DropLayerSet::addDropLayer", "[drop_layer_set]") {

    p4sfu::DropLayerSet r;
    CHECK_NOTHROW(r.addDropLayer(1));
    CHECK_NOTHROW(r.addDropLayer(3));
    CHECK_THROWS(r.addDropLayer(3));
    CHECK(r.dropsLayer(1));
    CHECK_FALSE(r.dropsLayer(2));
    CHECK(r.dropsLayer(3));
    CHECK_FALSE(r.dropsLayer(4));
}

TEST_CASE("DropLayerSet::removeDropLayer", "[drop_layer_set]") {

    p4sfu::DropLayerSet r;
    CHECK_NOTHROW(r.addDropLayer(1));
    CHECK_NOTHROW(r.addDropLayer(3));
    CHECK_NOTHROW(r.removeDropLayer(1));
    CHECK_NOTHROW(r.removeDropLayer(3));
    CHECK_THROWS(r.removeDropLayer(5));
    CHECK_FALSE(r.dropsLayer(1));
    CHECK_FALSE(r.dropsLayer(3));
}

#include <catch2/catch_test_macros.hpp>

#include "static_hash_map.hh"

namespace porpoise::tests {

TEST_CASE("Metadata helpers") {
    using detail::Metadata;
    Metadata metadata;

    metadata.open_up();
    CHECK(metadata.is_open());
    metadata.bury();
    CHECK(metadata.is_tombstone());

    metadata.fill(20);
    CHECK(metadata.get_fingerprint() == 20);
    CHECK(metadata.is_used());
    metadata.fill(255);
    REQUIRE(metadata.get_fingerprint() == 127);

    REQUIRE(Metadata::take_fingerprint(300) == 0);
    REQUIRE(Metadata::take_fingerprint(0xFFFFFFFFFFFFFFF) == 7);
    REQUIRE(Metadata::take_fingerprint(0xAFFF5FFFFFFFFFFF) == 87);
    REQUIRE(Metadata::take_fingerprint(0xFFFFFFFFFFFFFFFF) == 127);
}

} // namespace porpoise::tests

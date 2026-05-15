#include <catch2/catch_test_macros.hpp>

#include "math.hh"
#include "types.hh"

namespace porpoise::tests {

TEST_CASE("Ceil power of two") {
    SECTION("u8") {
        CHECK(ceil_power_of_two<u8>(0) == 1);
        CHECK(ceil_power_of_two<u8>(2) == 2);
        CHECK(ceil_power_of_two<u8>(3) == 4);
    }

    SECTION("u16") {
        CHECK(ceil_power_of_two<u16>(0) == 1);
        CHECK(ceil_power_of_two<u16>(2) == 2);
        CHECK(ceil_power_of_two<u16>(3) == 4);
        CHECK(ceil_power_of_two<u16>(4'097) == 8'192);
    }

    SECTION("u32") {
        CHECK(ceil_power_of_two<u32>(0) == 1);
        CHECK(ceil_power_of_two<u32>(2) == 2);
        CHECK(ceil_power_of_two<u32>(3) == 4);
        CHECK(ceil_power_of_two<u32>(25'702) == 32'768);
    }

    SECTION("u64") {
        CHECK(ceil_power_of_two<u64>(0) == 1);
        CHECK(ceil_power_of_two<u64>(2) == 2);
        CHECK(ceil_power_of_two<u64>(3) == 4);
        CHECK(ceil_power_of_two<u64>(257) == 512);
        CHECK(ceil_power_of_two<u64>(0xE00000000000000) == 0x1000000000000000);
        CHECK(ceil_power_of_two<u64>(0xF000000000000000) == 0);
    }
}

} // namespace porpoise::tests

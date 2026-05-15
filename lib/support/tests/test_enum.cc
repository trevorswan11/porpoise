#include <array>

#include <catch2/catch_test_macros.hpp>

#include "enum.hh"
#include "helpers/enum.hh"
#include "types.hh"

namespace porpoise::tests {

using helpers::MockEnum;
using helpers::MockNegativeEnum;
using helpers::MockPositiveEnum;
using helpers::NonMonotonicEnum;

TEST_CASE("Enum min/max calculations") {
    STATIC_CHECK(enum_min_underlying<MockEnum>() == 0);
    STATIC_CHECK(enum_max_underlying<MockEnum>() == 3);

    STATIC_CHECK(enum_min_underlying<MockPositiveEnum>() == 1);
    STATIC_CHECK(enum_max_underlying<MockPositiveEnum>() == 4);

    STATIC_CHECK(enum_min_underlying<MockNegativeEnum>() == -1);
    STATIC_CHECK(enum_max_underlying<MockNegativeEnum>() == 2);

    STATIC_CHECK(enum_min_underlying<NonMonotonicEnum>() == 0);
    STATIC_CHECK(enum_max_underlying<NonMonotonicEnum>() == 25);
}

TEST_CASE("Monotonically increasing enum range") {
    usize i = 0;
    for (const auto v : enum_range<MockEnum::A, MockEnum::D>()) {
        CHECK(v == static_cast<MockEnum>(i++));
    }

    i = 0;
    for (const auto v : enum_range<MockEnum>()) { CHECK(v == static_cast<MockEnum>(i++)); }
}

TEST_CASE("Non-monotonic enum range") {
    usize          i        = 0;
    constexpr auto expected = std::array{
        NonMonotonicEnum::A,
        NonMonotonicEnum::B,
        NonMonotonicEnum::D,
        NonMonotonicEnum::C,
    };
    for (const auto v : enum_range<NonMonotonicEnum::A, NonMonotonicEnum::D>()) {
        CHECK(v == expected[i++]);
    }

    i = 0;
    for (const auto v : enum_range<NonMonotonicEnum>()) { CHECK(v == expected[i++]); }
}

} // namespace porpoise::tests

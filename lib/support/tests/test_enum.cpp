#include <catch2/catch_test_macros.hpp>

#include "enum.hpp"
#include "optional.hpp"

namespace porpoise::tests {

enum class MockEnum : u8 {
    A,
    B,
    C,
    D,
};

enum class MockPositiveEnum : u8 {
    A = 1,
    B,
    C,
    D,
};

enum class MockNegativeEnum : i8 {
    A = -1,
    B,
    C,
    D,
};

enum class NonMonotonicEnum : u8 {
    A = 0,
    B = 10,
    C = 25,
    D = 23,
};

TEST_CASE("Enum min/max calculations") {
    STATIC_CHECK(enum_min<MockEnum>() == 0);
    STATIC_CHECK(enum_max<MockEnum>() == 3);

    STATIC_CHECK(enum_min<MockPositiveEnum>() == 1);
    STATIC_CHECK(enum_max<MockPositiveEnum>() == 4);

    STATIC_CHECK(enum_min<MockNegativeEnum>() == -1);
    STATIC_CHECK(enum_max<MockNegativeEnum>() == 2);

    STATIC_CHECK(enum_min<NonMonotonicEnum>() == 0);
    STATIC_CHECK(enum_max<NonMonotonicEnum>() == 25);
}

TEST_CASE("Standard enum map") {
    EnumMap<MockEnum, Optional<int>> map;
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK_FALSE(item); }

    map[MockEnum::A] = 4;
    for (const auto& item : map) {
        if (item) { CHECK(item == 4); }
    }

    SECTION("Automatic optional getting") {
        const auto present_opt = map.get_opt(MockEnum::A);
        CHECK(present_opt);
        CHECK(present_opt == 4);

        const auto missing_opt = map.get_opt(MockEnum::B);
        CHECK_FALSE(missing_opt);
    }
}

TEST_CASE("Positive enum map") {
    EnumMap<MockPositiveEnum, usize*> map;
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK(item == nullptr); }

    usize v                  = 1;
    map[MockPositiveEnum::A] = &v;
    for (const auto& item : map) {
        if (item != nullptr) { CHECK(*item == 1); }
    }

    SECTION("Automatic optional getting") {
        const auto present_opt = map.get_opt(MockPositiveEnum::A);
        CHECK(**present_opt == 1);

        const auto missing_opt = map.get_opt(MockPositiveEnum::B);
        CHECK_FALSE(missing_opt);
    }
}

TEST_CASE("Negative enum map") {
    EnumMap<MockNegativeEnum, bool> map{true};
    CHECK(map.size() == 4);
    for (const auto& item : map) { CHECK(item); }

    map[MockNegativeEnum::A] = false;
    CHECK_FALSE(map[MockNegativeEnum::A]);
    CHECK(map[MockNegativeEnum::B]);
    CHECK(map[MockNegativeEnum::C]);
    CHECK(map[MockNegativeEnum::D]);
}

TEST_CASE("Non-monotonic enum map") {
    EnumMap<NonMonotonicEnum, usize> map{0xDEADBEEF};
    CHECK(map.size() == 4);

    map[NonMonotonicEnum::D] = 0xC0FFEE;
    CHECK(map[NonMonotonicEnum::A] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::B] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::C] == 0xDEADBEEF);
    CHECK(map[NonMonotonicEnum::D] == 0xC0FFEE);
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

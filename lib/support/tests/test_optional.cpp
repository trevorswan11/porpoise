#include <algorithm>
#include <cctype>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"

#include "enum.hpp"
#include "option.hpp"
#include "types.hpp"

namespace porpoise {

namespace tests {

enum class NonOptionableEnum : u8 {
    A,
    B,
    C,
};

enum class OptionableEnum : u8 {
    A,
    B,
    C,
};

} // namespace tests

namespace opt {

template <> struct SentinelEnum<tests::OptionableEnum> {
    static constexpr auto SENTINEL = EnumLimits<tests::OptionableEnum>::max();
};

} // namespace opt

namespace tests {

TEST_CASE("Ref construction checks") {
    STATIC_CHECK_FALSE(std::is_constructible_v<opt::detail::Ref<i32>, i32&&>);
    STATIC_CHECK(TriviallyCopyable<opt::detail::Ref<i32>>);
}

TEST_CASE("opt::Option template specialization") {
    STATIC_CHECK(std::is_same_v<opt::Option<i32&>, opt::detail::Ref<i32>>);
    STATIC_CHECK(std::is_same_v<opt::Option<const i32&>, opt::detail::Ref<const i32>>);
    STATIC_CHECK(std::is_same_v<opt::Option<i32>, std::optional<i32>>);
}

TEST_CASE("opt::Option template checks") {
    STATIC_CHECK(opt::is_option<opt::Option<int>>::value);
    STATIC_CHECK(opt::is_option<opt::Option<int&>>::value);
    STATIC_CHECK(opt::is_option_v<opt::Option<int>>);
    STATIC_CHECK(opt::is_option_v<opt::Option<int&>>);
    STATIC_CHECK_FALSE(opt::is_option_v<int>);
}

TEST_CASE("Ref basic construction") {
    i32                         val = 42;
    const opt::detail::Ref<i32> opt{val};

    CHECK(opt.has_value());
    CHECK(static_cast<bool>(opt));
    CHECK(opt);
    CHECK(opt.value() == val);
    CHECK(*opt == 42);
    CHECK(&*opt == &val);
}

TEST_CASE("Ref null use & access") {
    SECTION("Default") {
        const opt::detail::Ref<i32> opt{};
        CHECK_FALSE(opt.has_value());
        CHECK_THROWS_AS(opt.value(), std::bad_optional_access);
    }

    SECTION("Explicit") {
        const opt::detail::Ref<i32> opt{opt::none};
        CHECK_FALSE(opt.has_value());
        CHECK_THROWS_AS(opt.value(), std::bad_optional_access);
    }
}

TEST_CASE("Ref conversions") {
    SECTION("Non-const -> const") {
        i32                               val = 42;
        const opt::detail::Ref<i32>       mut_opt{val};
        const opt::detail::Ref<const i32> const_opt{mut_opt};
        CHECK(const_opt.has_value());
        CHECK(*const_opt == 42);
    }

    SECTION("Derived -> Base") {
        helpers::Derived                         d;
        const opt::detail::Ref<helpers::Derived> d_opt{d};
        const opt::detail::Ref<helpers::Base>    base_opt = d_opt;
        CHECK(base_opt.has_value());
        CHECK(base_opt->x == 10);
    }
}

TEST_CASE("Ref reassignment") {
    i32                   a = 1, b = 2;
    opt::detail::Ref<i32> opt{a};
    CHECK(*opt == 1);

    opt = b;
    CHECK(*opt == 2);

    opt = opt::none;
    CHECK_FALSE(opt.has_value());
}

TEST_CASE("Ref mutability") {
    i32                         val = 42;
    const opt::detail::Ref<i32> opt{val};
    CHECK(*opt == 42);

    *opt = 1;
    CHECK(*opt == 1);
    CHECK(val == 1);
    CHECK(&*opt == &val);
}

TEST_CASE("Safe optional default equality") {
    i32                     x = 10, y = 10, z = 20;
    const opt::Option<i32&> opt_x{x};
    const opt::Option<i32&> opt_y{y};
    const opt::Option<i32&> opt_z{z};
    const opt::Option<i32&> opt_null;

    CHECK(opt::safe_eq<i32&>(opt_x, opt_y));
    CHECK_FALSE(opt::safe_eq<i32&>(opt_x, opt_z));
    CHECK_FALSE(opt::safe_eq<i32&>(opt_x, opt_null));
    CHECK(opt::safe_eq<i32&>(opt_null, opt_null));
}

TEST_CASE("Safe optional custom equality") {
    std::string                     s1 = "APPLE", s2 = "apple";
    const opt::Option<std::string&> opt1{s1};
    const opt::Option<std::string&> opt2{s2};

    CHECK(opt::safe_eq<std::string&>(opt1, opt2, [](const std::string& a, const std::string& b) {
        return std::ranges::equal(
            a, b, [](byte ac, byte bc) { return std::tolower(ac) == std::tolower(bc); });
    }));
}

TEST_CASE("Ref transform on value") {
    i32               i = 9;
    opt::Option<i32&> opt_i{i};

    const auto res = opt_i.transform([](const i32& i) { return i + 2; });
    REQUIRE(res);
    CHECK(*res == 11);
}

TEST_CASE("Ref transform on none") {
    opt::Option<i32&> opt_i{};

    const auto res = opt_i.transform([](const i32& i) { return i + 2; });
    CHECK_FALSE(res);
}

TEST_CASE("Boolean wrapper") {
    opt::detail::Boolean b;
    CHECK_FALSE(b.has_value());
    CHECK_THROWS_AS(b.value(), std::bad_optional_access);
    CHECK_FALSE(b.value_or(false));
    CHECK_FALSE(b.value_or(0));

    b = true;
    CHECK(b.has_value());
    CHECK(b.value_or(false));
}

TEST_CASE("Index wrapper") {
    opt::Index i;
    CHECK_FALSE(i.has_value());
    CHECK_THROWS_AS(i.value(), std::bad_optional_access);

    i = 0;
    CHECK(i.has_value());
    CHECK(*i == 0);
}

TEST_CASE("OptionableEnum concept") {
    STATIC_REQUIRE_FALSE(opt::OptionableEnum<i32>);
    STATIC_REQUIRE_FALSE(opt::OptionableEnum<NonOptionableEnum>);
    STATIC_REQUIRE(opt::OptionableEnum<OptionableEnum>);
    STATIC_REQUIRE(sizeof(opt::Enum<OptionableEnum>) == sizeof(OptionableEnum));
}

TEST_CASE("Optional enum wrapper") {
    opt::Enum<OptionableEnum> e;
    CHECK_FALSE(e.has_value());
    CHECK_THROWS_AS(e.value(), std::bad_optional_access);

    e = OptionableEnum::A;
    CHECK(e.has_value());
    CHECK(*e == OptionableEnum::A);
}

TEST_CASE("Enum transform on value") {
    opt::Enum<OptionableEnum> e{OptionableEnum::A};
    const auto res = e.transform([](const OptionableEnum&) { return OptionableEnum::B; });
    REQUIRE(res);
    CHECK(*res == OptionableEnum::B);
}

TEST_CASE("Enum transform on none") {
    opt::Enum<OptionableEnum> e{};
    const auto res = e.transform([](const OptionableEnum&) { return OptionableEnum::B; });
    CHECK_FALSE(res);
}

} // namespace tests

} // namespace porpoise

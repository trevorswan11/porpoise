#include <algorithm>
#include <cctype>
#include <string>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "memory.hpp"
#include "optional.hpp"
#include "types.hpp"

namespace porpoise::tests {

static_assert(!std::is_constructible_v<OptionalRef<int>, int&&>);
static_assert(std::is_trivially_copyable_v<OptionalRef<int>>);

static_assert(std::is_same_v<Optional<int&>, OptionalRef<int>>);
static_assert(std::is_same_v<Optional<const int&>, OptionalRef<const int>>);
static_assert(std::is_same_v<Optional<int>, std::optional<int>>);

TEST_CASE("OptRef basic construction") {
    int                    val = 42;
    const OptionalRef<int> opt{val};

    REQUIRE(opt.has_value());
    REQUIRE(static_cast<bool>(opt));
    REQUIRE(opt);
    REQUIRE(opt.value() == val);
    REQUIRE(*opt == 42);
    REQUIRE(&*opt == &val);
}

TEST_CASE("OptRef null use & access") {
    SECTION("Default") {
        const OptionalRef<int> opt{};
        REQUIRE_FALSE(opt.has_value());
        REQUIRE_THROWS(opt.value(), std::bad_optional_access{});
    }

    SECTION("Explicit") {
        const OptionalRef<int> opt{std::nullopt};
        REQUIRE_FALSE(opt.has_value());
        REQUIRE_THROWS(opt.value(), std::bad_optional_access{});
    }
}

TEST_CASE("OptRef conversions") {
    SECTION("Non-const -> const") {
        int                          val = 42;
        const OptionalRef<int>       mut_opt{val};
        const OptionalRef<const int> const_opt{mut_opt};
        REQUIRE(const_opt.has_value());
        REQUIRE(*const_opt == 42);
    }

    SECTION("Derived -> Base") {
        struct Base {
            virtual ~Base() = default;
            int x           = 10;
        };

        struct Derived : Base {
            int y = 20;
        };

        Derived                    d;
        const OptionalRef<Derived> d_opt{d};
        const OptionalRef<Base>    base_opt = d_opt;
        REQUIRE(base_opt.has_value());
        REQUIRE(base_opt->x == 10);
    }
}

TEST_CASE("OptRef reassignment") {
    int              a = 1, b = 2;
    OptionalRef<int> opt{a};
    REQUIRE(*opt == 1);

    opt = b;
    REQUIRE(*opt == 2);

    opt = std::nullopt;
    REQUIRE_FALSE(opt.has_value());
}

TEST_CASE("OptRef mutability") {
    int                    val = 42;
    const OptionalRef<int> opt{val};
    REQUIRE(*opt == 42);

    *opt = 1;
    REQUIRE(*opt == 1);
    REQUIRE(val == 1);
    REQUIRE(&*opt == &val);
}

TEST_CASE("Safe optional default equality") {
    int                  x = 10, y = 10, z = 20;
    const Optional<int&> opt_x{x};
    const Optional<int&> opt_y{y};
    const Optional<int&> opt_z{z};
    const Optional<int&> opt_null;

    REQUIRE(optional::safe_eq<int&>(opt_x, opt_y));
    REQUIRE_FALSE(optional::safe_eq<int&>(opt_x, opt_z));
    REQUIRE_FALSE(optional::safe_eq<int&>(opt_x, opt_null));
    REQUIRE(optional::safe_eq<int&>(opt_null, opt_null));
}

TEST_CASE("Safe optional custom equality") {
    std::string                  s1 = "APPLE", s2 = "apple";
    const Optional<std::string&> opt1{s1};
    const Optional<std::string&> opt2{s2};

    REQUIRE(
        optional::safe_eq<std::string&>(opt1, opt2, [](const std::string& a, const std::string& b) {
            return std::ranges::equal(
                a, b, [](byte ac, byte bc) { return std::tolower(ac) == std::tolower(bc); });
        }));
}

TEST_CASE("Unsafe optional default equality") {
    const Optional<Box<int>> opt_a{make_box<int>(100)};
    const Optional<Box<int>> opt_b{make_box<int>(100)};
    const Optional<Box<int>> opt_c{make_box<int>(200)};
    const Optional<Box<int>> opt_null;

    REQUIRE(optional::unsafe_eq<int>(opt_a, opt_b));
    REQUIRE_FALSE(optional::unsafe_eq<int>(opt_a, opt_c));
    REQUIRE_FALSE(optional::unsafe_eq<int>(opt_a, opt_null));
    REQUIRE(optional::unsafe_eq<int>(opt_null, opt_null));
}

TEST_CASE("Unsafe optional custom equality") {
    struct Node {
        usize            type_id;
        std::string_view name;
    };

    const Optional<Box<Node>> a = make_box<Node>(1, "foo");
    const Optional<Box<Node>> b = make_box<Node>(1, "bar");
    REQUIRE(optional::unsafe_eq<Node>(
        a, b, [](const Node& an, const Node& bn) { return an.type_id == bn.type_id; }));
}

} // namespace porpoise::tests

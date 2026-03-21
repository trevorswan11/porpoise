#include <algorithm>
#include <cctype>
#include <string>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "memory.hpp"
#include "optional.hpp"
#include "types.hpp"

namespace porpoise::tests {

struct Base {
    virtual ~Base() = default;
    int x           = 10;
};

struct Derived : Base {
    int y = 20;
};

TEST_CASE("OptRef construction checks") {
    STATIC_CHECK_FALSE(std::is_constructible_v<OptionalRef<int>, int&&>);
    STATIC_CHECK(std::is_trivially_copyable_v<OptionalRef<int>>);
}

TEST_CASE("Optional template specialization") {
    STATIC_CHECK(std::is_same_v<Optional<int&>, OptionalRef<int>>);
    STATIC_CHECK(std::is_same_v<Optional<const int&>, OptionalRef<const int>>);
    STATIC_CHECK(std::is_same_v<Optional<int>, std::optional<int>>);
}

TEST_CASE("OptRef basic construction") {
    int                    val = 42;
    const OptionalRef<int> opt{val};

    CHECK(opt.has_value());
    CHECK(static_cast<bool>(opt));
    CHECK(opt);
    CHECK(opt.value() == val);
    CHECK(*opt == 42);
    CHECK(&*opt == &val);
}

TEST_CASE("OptRef null use & access") {
    SECTION("Default") {
        const OptionalRef<int> opt{};
        CHECK_FALSE(opt.has_value());
        CHECK_THROWS(opt.value(), std::bad_optional_access{});
    }

    SECTION("Explicit") {
        const OptionalRef<int> opt{std::nullopt};
        CHECK_FALSE(opt.has_value());
        CHECK_THROWS(opt.value(), std::bad_optional_access{});
    }
}

TEST_CASE("OptRef conversions") {
    SECTION("Non-const -> const") {
        int                          val = 42;
        const OptionalRef<int>       mut_opt{val};
        const OptionalRef<const int> const_opt{mut_opt};
        CHECK(const_opt.has_value());
        CHECK(*const_opt == 42);
    }

    SECTION("Derived -> Base") {
        Derived                    d;
        const OptionalRef<Derived> d_opt{d};
        const OptionalRef<Base>    base_opt = d_opt;
        CHECK(base_opt.has_value());
        CHECK(base_opt->x == 10);
    }
}

TEST_CASE("OptRef reassignment") {
    int              a = 1, b = 2;
    OptionalRef<int> opt{a};
    CHECK(*opt == 1);

    opt = b;
    CHECK(*opt == 2);

    opt = std::nullopt;
    CHECK_FALSE(opt.has_value());
}

TEST_CASE("OptRef mutability") {
    int                    val = 42;
    const OptionalRef<int> opt{val};
    CHECK(*opt == 42);

    *opt = 1;
    CHECK(*opt == 1);
    CHECK(val == 1);
    CHECK(&*opt == &val);
}

TEST_CASE("Safe optional default equality") {
    int                  x = 10, y = 10, z = 20;
    const Optional<int&> opt_x{x};
    const Optional<int&> opt_y{y};
    const Optional<int&> opt_z{z};
    const Optional<int&> opt_null;

    CHECK(optional::safe_eq<int&>(opt_x, opt_y));
    CHECK_FALSE(optional::safe_eq<int&>(opt_x, opt_z));
    CHECK_FALSE(optional::safe_eq<int&>(opt_x, opt_null));
    CHECK(optional::safe_eq<int&>(opt_null, opt_null));
}

TEST_CASE("Safe optional custom equality") {
    std::string                  s1 = "APPLE", s2 = "apple";
    const Optional<std::string&> opt1{s1};
    const Optional<std::string&> opt2{s2};

    CHECK(
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

    CHECK(optional::unsafe_eq<int>(opt_a, opt_b));
    CHECK_FALSE(optional::unsafe_eq<int>(opt_a, opt_c));
    CHECK_FALSE(optional::unsafe_eq<int>(opt_a, opt_null));
    CHECK(optional::unsafe_eq<int>(opt_null, opt_null));
}

TEST_CASE("Unsafe optional custom equality") {
    struct Node {
        usize            type_id;
        std::string_view name;
    };

    const Optional<Box<Node>> a = make_box<Node>(1, "foo");
    const Optional<Box<Node>> b = make_box<Node>(1, "bar");
    CHECK(optional::unsafe_eq<Node>(
        a, b, [](const Node& an, const Node& bn) { return an.type_id == bn.type_id; }));
}

TEST_CASE("NonNull construction checks") {
    STATIC_CHECK_FALSE(std::is_constructible_v<NonNull<int>, int&&>);
    STATIC_CHECK_FALSE(std::is_constructible_v<NonNull<int>, std::nullopt_t>);
    STATIC_CHECK(std::is_trivially_copyable_v<NonNull<int>>);
}

TEST_CASE("NonNull basic usage") {
    int                val = 42;
    const NonNull<int> ptr{&val};

    CHECK(*ptr == 42);
    CHECK(ptr.get() == &val);
    CHECK(static_cast<int>(ptr) == 42);
}

TEST_CASE("NonNull from OptRef") {
    int                    val = 10;
    const OptionalRef<int> opt{val};

    const NonNull<int> ptr{opt};
    CHECK(*ptr == 10);

    const OptionalRef<int> empty;
    CHECK_THROWS(NonNull<int>{empty}, std::bad_optional_access{});
}

TEST_CASE("NonNull conversions") {
    Derived                d;
    const NonNull<Derived> d_ptr(&d);

    NonNull<Base> b_ptr{d_ptr};
    CHECK(b_ptr->x == 10);
    NonNull<const Derived> cd_ptr{d_ptr};
    CHECK(cd_ptr->y == 20);
}

} // namespace porpoise::tests

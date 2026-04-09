#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"

#include "memory.hpp"
#include "types.hpp"

namespace porpoise::tests {

using OkNullableBox     = mem::NullableBox<bool>;
using NotNullableBox    = bool;
using CustomNullableBox = mem::NullableBox<bool, void (*)(bool*)>;

TEST_CASE("Nullable box template checks") {
    STATIC_CHECK(mem::is_nullable_box<std::unique_ptr<bool>>::value);
    STATIC_CHECK(mem::is_nullable_box_v<OkNullableBox>);
    STATIC_CHECK_FALSE(mem::is_nullable_box_v<NotNullableBox>);
    STATIC_CHECK(mem::is_nullable_box_v<CustomNullableBox>);
}

using OkBox     = mem::Box<bool>;
using NotBox    = bool;
using CustomBox = mem::Box<bool, void (*)(bool*)>;

TEST_CASE("Basic box construction") {
    auto b = mem::make_box<int>(42);
    CHECK(*b == 42);
}

TEST_CASE("Box invariant enforcement") {
    mem::NullableBox<int> n;
    CHECK_THROWS_AS(mem::Box<int>{std::move(n)}, mem::NullBoxException);
    int* p = nullptr;
    CHECK_THROWS_AS(mem::Box<int>{p}, mem::NullBoxException);
}

TEST_CASE("Box upcasting") {
    auto           d = mem::make_box<Derived>();
    mem::Box<Base> b = std::move(d);
    CHECK(b->x == 10);
}

TEST_CASE("Box release") {
    auto b   = mem::make_box<int>(10);
    int* raw = b.release();
    CHECK(raw != nullptr);
    delete raw;
}

TEST_CASE("Box template checks") {
    STATIC_CHECK(sizeof(mem::Box<int>) == 8);
    STATIC_CHECK_FALSE(mem::is_box<std::unique_ptr<bool>>::value);
    STATIC_CHECK(mem::is_box_v<OkBox>);
    STATIC_CHECK_FALSE(mem::is_box_v<NotBox>);
    STATIC_CHECK(mem::is_box_v<CustomBox>);
}

TEST_CASE("Unsafe optional default equality") {
    const Optional<mem::Box<i32>> opt_a{mem::make_box<i32>(100)};
    const Optional<mem::Box<i32>> opt_b{mem::make_box<i32>(100)};
    const Optional<mem::Box<i32>> opt_c{mem::make_box<i32>(200)};
    const Optional<mem::Box<i32>> opt_null;

    CHECK(optional::unsafe_eq<i32>(opt_a, opt_b));
    CHECK_FALSE(optional::unsafe_eq<i32>(opt_a, opt_c));
    CHECK_FALSE(optional::unsafe_eq<i32>(opt_a, opt_null));
    CHECK(optional::unsafe_eq<i32>(opt_null, opt_null));
}

TEST_CASE("Unsafe optional custom equality") {
    struct Node {
        usize            type_id;
        std::string_view name;
    };

    const Optional<mem::Box<Node>> a = mem::make_box<Node>(1, "foo");
    const Optional<mem::Box<Node>> b = mem::make_box<Node>(1, "bar");
    CHECK(optional::unsafe_eq<Node>(
        a, b, [](const Node& an, const Node& bn) { return an.type_id == bn.type_id; }));
}

} // namespace porpoise::tests

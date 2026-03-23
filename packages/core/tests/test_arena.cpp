#include <catch2/catch_test_macros.hpp>

#include "arena.hpp"

namespace porpoise::tests {

constexpr usize MARKER{42};

struct Foo {
    usize                      marker{MARKER};
    std::array<i32, 4 * 1'024> _;
};

TEST_CASE("Arena pointer stability") {
    mem::Arena        arena;
    std::vector<Foo*> foos;

    for (usize i = 0; i < 100; ++i) { foos.emplace_back(arena.make<Foo>().get()); }
    for (const auto& foo : foos) { CHECK(foo->marker == MARKER); }

    SECTION("Sneaky non-UB use after free") {
        arena.reset();
        CHECK(foos[0]->marker == MARKER);
    }
}

TEST_CASE("Arena alignment") {
    mem::Arena arena;
    CHECK(*arena.make<bool>(true));
    const auto p = arena.make<void*>(nullptr);
    CHECK(reinterpret_cast<uintptr_t>(p.get()) % alignof(void*) == 0);
}

TEST_CASE("Arena array construction") {
    mem::Arena arena;
    const auto array = arena.make_span<i32>(10);
    for (const auto& i : array) { CHECK(i == 0); }
}

} // namespace porpoise::tests

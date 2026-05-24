#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("Function declaration") {
    auto [ctx, idx] = helpers::resolve_and_check(R"(
        const a := fn(b: i32, c: *@typeOf(b), d: [:0]u8): bool {
            return true;
        };

        const int: i32 = 12;
        const result: bool = a(1, ^int, "Hello, World");
    )");
}

} // namespace porpoise::tests

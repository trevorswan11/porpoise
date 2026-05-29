#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("For loop resolution") {
    auto [ctx, idx] = helpers::resolve_and_check(
        "const a := for (0..5, blk: { break :blk 2..4; }) |i, j| { const foo := 42; }"
        "else { const foo := 42; };");
}

TEST_CASE("Trivial loop resolution") {
    helpers::resolve_and_check(
        "const a := do { const foo := 42; } while (blk: { break :blk 42; });");
    helpers::resolve_and_check("const a := loop { const foo := 42; };");
    helpers::resolve_and_check(
        "var i: i32; const a := while (blk: { break :blk 42; }) : (i += blk: { break "
        ":blk 42; }) { const foo := 42; } else { const foo := 42; };");
}

} // namespace porpoise::tests

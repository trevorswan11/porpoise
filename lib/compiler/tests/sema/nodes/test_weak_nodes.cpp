#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

TEST_CASE("Array/Index collection") {
    helpers::analyze_and_validate("const a := [2uz]i32{A, B, }; const b := a[0];");
    helpers::analyze_and_validate("const a := [_]*N{a, b, c, d, e, }; const b := a[2];");
    helpers::analyze_and_validate("const a := [_]*N{a, b, c, d, if (e > f) g else h, };");
    helpers::analyze_and_validate("const a := [2uz]i32{A, match (B) { c => d }, };");
}

TEST_CASE("Builtin calling collection") {
    for (const auto& builtin : syntax::ALL_BUILTINS) {
        const auto input = fmt::format(
            "const a := {}(match (b) {{ c => d e => f}}, g, if (5 <= h) i else j);", builtin.first);
        helpers::analyze_and_validate(input);
    }
}

TEST_CASE("Initializer collection") {
    helpers::analyze_and_validate("const a := T{ .b = 2 };");
    helpers::analyze_and_validate("const a: T = .{ .b = 2 };");
    helpers::analyze_and_validate("const a := T{ .b = 3u + v, .c = if (5 <= h) i else j };");
}

TEST_CASE("Discard statement collection") {
    helpers::analyze_and_validate("_ = 0..20;");
    helpers::analyze_and_validate("const a := fn(&self, c: type): void { _ = c; };");
}

TEST_CASE("Prefix expression collection") {
    helpers::analyze_and_validate("const a := &b;");
    helpers::analyze_and_validate("const a := *b;");
    helpers::analyze_and_validate("const a := !b;");
}

} // namespace porpoise::tests

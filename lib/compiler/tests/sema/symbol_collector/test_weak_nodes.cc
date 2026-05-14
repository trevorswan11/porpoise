#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

#include "syntax/builtins.hh"

namespace porpoise::tests {

TEST_CASE("Array/Index collection") {
    helpers::collect_and_check("const a := [2uz]i32{A, B, }; const b := a[0];");
    helpers::collect_and_check("const a := [_]*N{a, b, c, d, e, }; const b := a[2];");
    helpers::collect_and_check("const a := [_]*N{a, b, c, d, if (e > f) g else h, };");
    helpers::collect_and_check("const a := [2uz]i32{A, match (B) { c => d }, };");
}

TEST_CASE("Builtin calling collection") {
    for (const auto& builtin : syntax::builtins::ALL_TOKEN_TYPES) {
        const auto input =
            fmt::format("const a := {}(match (b) {{ c => d e => f}}, g, if (5 <= h) i else j);",
                        *get_builtin_opt(builtin));
        helpers::collect_and_check(input);
    }
}

TEST_CASE("Initializer collection") {
    helpers::collect_and_check("const a := T{ .b = 2 };");
    helpers::collect_and_check("const a: T = .{ .b = 2 };");
    helpers::collect_and_check("const a := T{ .b = 3u + v, .c = if (5 <= h) i else j };");
}

TEST_CASE("Discard statement collection") {
    helpers::collect_and_check("_ = 0..20;");
    helpers::collect_and_check("const a := fn(&self, c: type): void { _ = c; };");
}

TEST_CASE("Prefix expression collection") {
    helpers::collect_and_check("const a := &b;");
    helpers::collect_and_check("const a := *b;");
    helpers::collect_and_check("const a := !b;");
}

} // namespace porpoise::tests

#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"

#include "types.hh"

namespace porpoise::tests {

namespace {

// Checks that the scope has the foo-bar decl
auto test_conditional_scope(helpers::SemaTestContext& ctx, usize idx) {
    auto&       registry = ctx.analyzer.get_registry();
    const auto& table    = helpers::unwrap(registry.get_opt(idx));
    CHECK(table.size() == 1);
    ctx.test_common_decl_collection(idx);
}

} // namespace

TEST_CASE("If expression collection") {
    auto [ctx, idx] = helpers::collect_and_check(
        "const a := if (b) { const foo := bar; } else { const foo := bar; };");

    auto& analyzer = ctx->analyzer;
    CHECK(analyzer.get_registry().size() == 3);
    const auto& actual = helpers::unwrap(analyzer.get_table_opt(idx));
    CHECK(actual.size() == 1);
    REQUIRE(actual.get_opt("a"));

    test_conditional_scope(*ctx, 1);
    test_conditional_scope(*ctx, 2);
}

TEST_CASE("Flat if collection") { helpers::collect_and_check("const a := if (b > 4) c; else d;"); }

TEST_CASE("Match expression collection") {
    auto [ctx, idx] =
        helpers::collect_and_check("const a := match (b) { c => |d| { const foo := bar; } e => "
                                   "|_| { const foo := bar; } } else { const foo := bar; };");

    auto& analyzer = ctx->analyzer;
    CHECK(analyzer.get_registry().size() == 6);
    const auto& actual = helpers::unwrap(analyzer.get_table_opt(idx));
    CHECK(actual.size() == 1);
    REQUIRE(actual.get_opt("a"));

    test_conditional_scope(*ctx, 2);
    test_conditional_scope(*ctx, 4);
    test_conditional_scope(*ctx, 5);
}

TEST_CASE("Flat match collection") {
    helpers::collect_and_check("const a := match (b) { c => |d| d; e => |_| f; g => h; } else i;");
}

TEST_CASE("If expression inner shadowing") {
    helpers::test_collector_fail(
        "const a := if (b) { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 20uz}});

    helpers::test_collector_fail(
        "const a := if (b) { var c: i32; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 41uz}});
}

TEST_CASE("Match shadowing assignee") {
    helpers::test_collector_fail(
        "const a := match (c) { b => |a| b; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 29uz}});

    helpers::test_collector_fail(
        "const a := match (c) { b => { var a: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 30uz}});

    helpers::test_collector_fail(
        "const a := match (b) { c => d; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 40uz}});
}

TEST_CASE("Match dispatch shadowing") {
    helpers::test_collector_fail(
        "const a := match (c) { b => |c| { var c: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'c'. Previous declaration here: 1:30",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 34uz}});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

namespace helpers {

// Checks that the scope has the foo-bar decl
auto test_conditional_scope(SemaTestContext& ctx, usize idx) {
    auto& registry = ctx.analyzer.get_registry();
    CHECK(registry.get(idx).size() == 1);
    ctx.test_common_decl_collection(idx);
}

} // namespace helpers

TEST_CASE("If expression collection") {
    auto [ctx, idx] = helpers::collect_and_check(
        "const a := if (b) { const foo := bar; } else { const foo := bar; };");

    auto& analyzer = ctx.analyzer;
    CHECK(analyzer.get_registry().size() == 3);
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == 1);

    const std::string_view name = "a";
    const auto             opt  = actual.get_opt(name);
    REQUIRE(opt);

    helpers::test_conditional_scope(ctx, 1);
    helpers::test_conditional_scope(ctx, 2);
}

TEST_CASE("Flat if collection") { helpers::collect_and_check("const a := if (b > 4) c; else d;"); }

TEST_CASE("Match expression collection") {
    auto [ctx, idx] =
        helpers::collect_and_check("const a := match (b) { c => |d| { const foo := bar; } e => "
                                   "|_| { const foo := bar; } } else { const foo := bar; };");

    auto& analyzer = ctx.analyzer;
    CHECK(analyzer.get_registry().size() == 6);
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == 1);

    const std::string_view name = "a";
    const auto             opt  = actual.get_opt(name);
    REQUIRE(opt);

    helpers::test_conditional_scope(ctx, 2);
    helpers::test_conditional_scope(ctx, 4);
    helpers::test_conditional_scope(ctx, 5);
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
                         std::pair{0uz, 23uz}});

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
        sema::Diagnostic{"Attempt to shadow identifier 'c'. Previous declaration here: 1:24",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 34uz}});
}

} // namespace porpoise::tests

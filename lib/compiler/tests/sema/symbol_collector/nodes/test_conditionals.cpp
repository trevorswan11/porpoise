#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;

namespace helpers {

// Checks that the scope has the foo-bar decl
auto test_conditional_scope(sema::Analyzer& analyzer, usize idx) {
    auto& registry = analyzer.get_registry();
    CHECK(registry.get(idx).size() == 1);

    const auto   name          = "foo";
    const auto   expected_node = helpers::foo_bar_decl();
    sema::Symbol expected{name, &expected_node};
    CHECK(expected == registry.get_from(idx, name));
}

} // namespace helpers

TEST_CASE("If expression collection") {
    auto [analyzer, idx] = helpers::collect_and_validate(
        "const a := if (b) { const foo := bar; } else { const foo := bar; };");
    CHECK(analyzer.get_registry().size() == 3);
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == 1);

    const std::string_view name = "a";
    const auto             opt  = actual.get_opt(name);
    REQUIRE(opt);

    // Set the semantic type of the consequence
    auto cons = helpers::make_block_stmt(helpers::foo_bar_decl());
    auto type = analyzer.get_pool().get({sema::TypeKind::BLOCK, false, 1});
    REQUIRE(type);
    cons->set_sema_type(*type);

    // Set the semantic type of the alternate
    auto alt = helpers::make_block_stmt<true>(helpers::foo_bar_decl());
    type     = analyzer.get_pool().get({sema::TypeKind::BLOCK, false, 2});
    REQUIRE(type);
    alt->set_sema_type(*type);

    // Create the symbol to compare against
    const ast::DeclStatement decl{
        syntax::Token{keywords::CONSTANT},
        helpers::make_ident("a"),
        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
        mem::make_nullable_box<ast::IfExpression>(syntax::Token{keywords::IF},
                                                  false,
                                                  helpers::make_ident("b"),
                                                  std::move(cons),
                                                  std::move(alt)),
        ast::DeclModifiers::CONSTANT,
    };
    sema::Symbol expected{name, &decl};
    CHECK(*opt == expected);

    helpers::test_conditional_scope(analyzer, 1);
    helpers::test_conditional_scope(analyzer, 2);
}

TEST_CASE("Flat if collection") {
    helpers::collect_and_validate("const a := if (b > 4) c; else d;");
}

TEST_CASE("Match expression collection") {
    auto [analyzer, idx] =
        helpers::collect_and_validate("const a := match (b) { c => |d| { const foo := bar; } e => "
                                      "|_| { const foo := bar; } } else { const foo := bar; };");
    CHECK(analyzer.get_registry().size() == 6);
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == 1);

    const std::string_view name = "a";
    const auto             opt  = actual.get_opt(name);
    REQUIRE(opt);

    const auto make_match_block = [&]<bool Nullable = false>(usize table_idx) {
        auto       blk  = helpers::make_block_stmt<Nullable>(helpers::foo_bar_decl());
        const auto type = analyzer.get_pool().get({sema::TypeKind::BLOCK, false, table_idx});
        REQUIRE(type);
        blk->set_sema_type(*type);
        return blk;
    };

    // Create the first arm with semantic info
    auto type = analyzer.get_pool().get({sema::TypeKind::MATCH_ARM, false, 1});
    REQUIRE(type);
    ast::MatchArm arm1{helpers::make_ident("c"), helpers::make_ident("d"), make_match_block(2)};
    arm1.set_sema_type(*type);

    // Create the second arm with semantic info
    type = analyzer.get_pool().get({sema::TypeKind::MATCH_ARM, false, 3});
    REQUIRE(type);
    ast::MatchArm arm2{helpers::make_ident("e"), Unit{}, make_match_block(4)};
    arm2.set_sema_type(*type);

    // Create the symbol to compare against
    const ast::DeclStatement decl{
        syntax::Token{keywords::CONSTANT},
        helpers::make_ident("a"),
        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
        mem::make_nullable_box<ast::MatchExpression>(
            syntax::Token{keywords::MATCH},
            helpers::make_ident("b"),
            helpers::make_vector<ast::MatchArm>(std::move(arm1), std::move(arm2)),
            make_match_block.template operator()<true>(5)),
        ast::DeclModifiers::CONSTANT,
    };
    sema::Symbol expected{name, &decl};
    CHECK(*opt == expected);

    helpers::test_conditional_scope(analyzer, 2);
    helpers::test_conditional_scope(analyzer, 4);
    helpers::test_conditional_scope(analyzer, 5);
}

TEST_CASE("Flat match collection") {
    helpers::collect_and_validate(
        "const a := match (b) { c => |d| d; e => |_| f; g => h; } else i;");
}

TEST_CASE("If expression inner shadowing") {
    helpers::test_collector_fail(
        "const a := if (b) { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 21uz}});

    helpers::test_collector_fail(
        "const a := if (b) { var c: i32; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 42uz}});
}

TEST_CASE("Match shadowing assignee") {
    helpers::test_collector_fail(
        "const a := match (c) { b => |a| b; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 24uz}});

    helpers::test_collector_fail(
        "const a := match (c) { b => { var a: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 31uz}});

    helpers::test_collector_fail(
        "const a := match (b) { c => d; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 41uz}});
}

TEST_CASE("Match dispatch shadowing") {
    helpers::test_collector_fail(
        "const a := match (c) { b => |c| { var c: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'c'. Previous declaration here: [1, 24]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 35uz}});
}

} // namespace porpoise::tests

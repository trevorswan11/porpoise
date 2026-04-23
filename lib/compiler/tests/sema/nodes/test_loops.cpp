#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;

TEST_CASE("Do-while loop collection") {
    auto analyzer = helpers::test_collector(
        "const a := do { const foo := bar; } while (true);",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::DoWhileLoopExpression>(
                    syntax::Token{keywords::DO},
                    helpers::make_block_stmt(helpers::foo_bar_decl()),
                    helpers::make_primitive<ast::BoolExpression>(true)),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            sema::types::Key{sema::TypeKind::BLOCK, false, 1}});

    helpers::test_hollow_symbols(analyzer, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("For loop collection") {
    const auto capture = [] {
        return ast::ForLoopCapture{ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}};
    };

    auto analyzer = helpers::test_collector(
        "const a := for (0..5) |i| { const foo := bar; } else c;",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::ForLoopExpression>(
                    syntax::Token{keywords::FOR},
                    helpers::make_vector<mem::Box<ast::Expression>>(
                        mem::make_box<ast::RangeExpression>(
                            syntax::Token{syntax::TokenType::INT_10, "0"},
                            helpers::make_primitive<ast::I32Expression>("0"),
                            syntax::TokenType::DOT_DOT,
                            helpers::make_primitive<ast::I32Expression>("5"))),
                    helpers::make_vector<ast::ForLoopCapture>(capture()),
                    helpers::make_block_stmt(helpers::foo_bar_decl()),
                    helpers::make_expr_stmt<true>(helpers::ident_from("c"))),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            sema::types::Key{sema::TypeKind::BLOCK, false, 1}});

    helpers::test_hollow_symbols(analyzer,
                                 helpers::TableEntry{"i", capture()},
                                 helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Infinite loop collection") {
    auto analyzer = helpers::test_collector(
        "const a := loop { const foo := bar; };",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::InfiniteLoopExpression>(
                    syntax::Token{keywords::LOOP},
                    helpers::make_block_stmt(helpers::foo_bar_decl())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            sema::types::Key{sema::TypeKind::BLOCK, false, 1}});

    helpers::test_hollow_symbols(analyzer, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("While loop collection") {
    auto analyzer = helpers::test_collector(
        "const a := while (true) : (i += 1) { const foo := bar; } else c;",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::WhileLoopExpression>(
                    syntax::Token{keywords::WHILE},
                    helpers::make_primitive<ast::BoolExpression>(true),
                    mem::make_nullable_box<ast::AssignmentExpression>(
                        syntax::Token{syntax::TokenType::IDENT, "i"},
                        helpers::make_ident("i"),
                        syntax::TokenType::PLUS_ASSIGN,
                        helpers::make_primitive<ast::I32Expression>("1")),
                    helpers::make_block_stmt(helpers::foo_bar_decl()),
                    helpers::make_expr_stmt<true>(helpers::ident_from("c"))),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            sema::types::Key{sema::TypeKind::BLOCK, false, 1}});

    helpers::test_hollow_symbols(analyzer, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Well-placed loop control flow") {
    SECTION("For loops") {
        helpers::analyze_and_validate("const a := for (0..5) |i| { break; };");
        helpers::analyze_and_validate("const a := for (0..5) |i| { continue; };");
    }

    SECTION("Do-while loop") {
        helpers::analyze_and_validate("const a := do { break; } while (true);");
        helpers::analyze_and_validate("const a := do { continue; } while (true);");
    }

    SECTION("Infinite loop") {
        helpers::analyze_and_validate("const a := loop { break; };");
        helpers::analyze_and_validate("const a := loop { continue; };");
    }

    SECTION("While loops") {
        helpers::analyze_and_validate("const a := while (true) { break; };");
        helpers::analyze_and_validate("const a := while (true) { continue; };");
    }
}

TEST_CASE("Non-break collected as separate scope") {
    helpers::analyze_and_validate(
        "const a := for (0..5) |i| { const foo := bar; } else { const foo := bar; };");

    helpers::analyze_and_validate(
        "const a := while (true) : (i += 1) { const foo := bar; } else { const foo := bar; };");
}

TEST_CASE("Non-break collection shadowing") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 56uz}});

    helpers::test_collector_fail(
        "const a := while (true) : (i += 1) { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 65uz}});
}

TEST_CASE("Shadowing in loops") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 29uz}});

    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var i: i32; };",
        sema::Diagnostic{"Redeclaration of symbol 'i'. Previous declaration here: [1, 24]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 29uz}});

    helpers::test_collector_fail(
        "const a := loop { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 19uz}});

    helpers::test_collector_fail(
        "const a := while (true) { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 27uz}});
}

} // namespace porpoise::tests

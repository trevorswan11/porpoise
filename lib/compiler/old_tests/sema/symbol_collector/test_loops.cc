#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mut       = helpers::mut;

TEST_CASE("Do-while loop collection") {
    auto ctx = helpers::test_collector(
        "const a := do { const foo := bar; } while (true);",
        {},
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
            sema::types::Key{sema::TypeKind::BLOCK, mut::IMMUTABLE, 1}});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("For loop collection") {
    const auto capture = [] {
        return ast::ForLoopCapture{ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}};
    };

    auto ctx = helpers::test_collector(
        "const a := for (0..5) |i| { const foo := bar; } else c;",
        {},
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
            sema::types::Key{sema::TypeKind::BLOCK, mut::IMMUTABLE, 1}});

    helpers::test_hollow_symbols(ctx,
                                 helpers::TableEntry{"i", capture()},
                                 helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Infinite loop collection") {
    auto ctx = helpers::test_collector(
        "const a := loop { const foo := bar; };",
        {},
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
            sema::types::Key{sema::TypeKind::BLOCK, mut::IMMUTABLE, 1}});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("While loop collection") {
    auto ctx = helpers::test_collector(
        "const a := while (true) : (i += 1) { const foo := bar; } else c;",
        {},
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
            sema::types::Key{sema::TypeKind::BLOCK, mut::IMMUTABLE, 1}});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

} // namespace porpoise::tests

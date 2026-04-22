#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"
#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;

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

} // namespace porpoise::tests

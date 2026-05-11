#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;
namespace mut       = helpers::mut;

TEST_CASE("Function hollow types") {
    const sema::types::Key key{sema::TypeKind::FUNCTION, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := fn(&self, c: type): void { const foo := bar; };",
        {},
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::FunctionExpression>(
                    syntax::Token{keywords::FN},
                    ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                    helpers::make_parameters(ast::FunctionParameter{
                        helpers::make_ident("c"), {mods::BASE, helpers::make_ident("type")}}),
                    false,
                    ast::ExplicitType{mods::BASE, helpers::make_ident("void")},
                    helpers::make_block_stmt<true>(helpers::foo_bar_decl())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::CALLABLE});

    helpers::test_hollow_symbols(
        ctx,
        helpers::TableEntry{"foo", helpers::foo_bar_decl()},
        helpers::TableEntry{"self", ast::SelfParameter{mods::REF, helpers::make_ident("self")}},
        helpers::TableEntry{"c",
                            ast::FunctionParameter{helpers::make_ident("c"),
                                                   {mods::BASE, helpers::make_ident("type")}}});
}

} // namespace porpoise::tests

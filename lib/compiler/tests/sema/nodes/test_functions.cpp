#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Function hollow types") {
    const sema::types::Key key{sema::TypeKind::FUNCTION, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := fn(&self, c: type): void { const foo := bar; };",
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
            key});

    helpers::test_hollow_symbols(
        analyzer,
        helpers::TableEntry{"foo", helpers::foo_bar_decl()},
        helpers::TableEntry{"self", ast::SelfParameter{mods::REF, helpers::make_ident("self")}},
        helpers::TableEntry{"c",
                            ast::FunctionParameter{helpers::make_ident("c"),
                                                   {mods::BASE, helpers::make_ident("type")}}});
}

TEST_CASE("Function basic param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f: bool): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Function self param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Function local param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(a, a: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: [1, 15]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 18uz}});
}

TEST_CASE("Function block shadowing") {
    helpers::test_collector_fail(
        "const f := fn(): void { var f := 3; };",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 25uz}});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Function collection") {
    helpers::test_collector(
        R"(pub const main := fn(args: [][:0]u8): i32 { const message := "Hello, world!"; };)",
        helpers::TableEntry{
            "main",
            ast::DeclStatement{
                syntax::Token{keywords::PUBLIC},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::FunctionExpression>(
                    syntax::Token{keywords::FN},
                    opt::none,
                    helpers::make_parameters(ast::FunctionParameter{
                        helpers::make_ident("args"),
                        ast::ExplicitType{
                            mods::BASE,
                            ast::ExplicitArrayType{
                                {},
                                false,
                                mem::make_box<ast::ExplicitType>(
                                    mods::BASE,
                                    ast::ExplicitArrayType{
                                        {},
                                        true,
                                        mem::make_box<ast::ExplicitType>(
                                            mods::BASE, helpers::make_ident("u8"))})}}}),
                    false,
                    ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                    helpers::make_block_stmt<true>(
                        helpers::foo_bar_decl())), // TODO: Abstract message decl
                ast::DeclModifiers::PUBLIC | ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::BLOCK, false, 1}});
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

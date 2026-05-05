#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Basic struct") {
    helpers::test_expr_stmt(
        "struct { var a: Foo = bar; const b := fn(*mut this, a: A, b: *B): C { c; }; };",
        ast::StructExpression{
            syntax::Token{keywords::STRUCT},
            helpers::make_members(
                ast::DeclStatement{
                    syntax::Token{keywords::VAR},
                    helpers::make_ident("a"),
                    mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                                       ast::ExplicitType{
                                                           mods::BASE,
                                                           helpers::make_ident("Foo"),
                                                       }),
                    helpers::make_ident<true>("bar"),
                    ast::DeclModifiers::VARIABLE,
                },
                ast::DeclStatement{
                    syntax::Token{keywords::CONSTANT},
                    helpers::make_ident("b"),
                    mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                    mem::make_nullable_box<ast::FunctionExpression>(
                        syntax::Token{keywords::FN},
                        ast::SelfParameter{mods::MUT_PTR, helpers::make_ident("this")},
                        helpers::make_parameters(
                            ast::FunctionParameter{helpers::make_ident("a"),
                                                   {mods::BASE, helpers::make_ident("A")}},
                            ast::FunctionParameter{helpers::make_ident("b"),
                                                   {mods::PTR, helpers::make_ident("B")}}),
                        false,
                        ast::ExplicitType{mods::BASE, helpers::make_ident("C")},
                        helpers::make_expr_block_stmt<true>(helpers::ident_from("c"))),
                    ast::DeclModifiers::CONSTANT,
                })});
}

TEST_CASE("Non-decl struct members") {
    helpers::test_expr_stmt(
        "struct { import std; using I = i32; };",
        ast::StructExpression{
            syntax::Token{keywords::STRUCT},
            helpers::make_members(
                ast::ImportStatement{syntax::Token{keywords::IMPORT},
                                     ast::LibraryImport{helpers::make_ident("std"), {}}},
                ast::UsingStatement{syntax::Token{keywords::USING},
                                    helpers::make_ident("I"),
                                    ast::ExplicitType{
                                        mods::BASE,
                                        helpers::make_ident("i32"),
                                    }})});
}

TEST_CASE("Empty struct body") {
    helpers::test_expr_stmt(
        "struct {};",
        ast::StructExpression{syntax::Token{keywords::STRUCT}, helpers::make_members()});
}

TEST_CASE("Illegal members") {
    helpers::test_parser_fail("struct { const foo := bar; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 9});

    helpers::test_parser_fail("struct { extern var foo: bar; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 9});
}

} // namespace porpoise::tests

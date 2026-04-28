#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Struct flavors") {
    const std::string_view input{
        "{ var a: Foo = bar; const b := fn(*mut this, a: A, b: *B): C { c; }; };"};
    const auto cases = std::to_array<std::pair<std::string, syntax::Keyword>>({
        {fmt::format("struct {}", input), keywords::STRUCT},
        {fmt::format("packed struct {}", input), keywords::PACKED},
    });

    for (const auto& input_case : cases) {
        helpers::test_expr_stmt(
            input_case.first,
            ast::StructExpression{
                syntax::Token{input_case.second},
                helpers::make_decls(
                    ast::DeclStatement{
                        syntax::Token{keywords::VAR},
                        helpers::make_ident("a"),
                        mem::make_box<ast::TypeExpression>(
                            syntax::Token{syntax::TokenType::COLON, ":"},
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
                        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS},
                                                           opt::none),
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
}

TEST_CASE("Illegal struct member") {
    helpers::test_parser_fail("struct { import std; };",
                              syntax::ParserDiagnostic{syntax::ParserError::INVALID_MEMBER, 0, 9});
}

TEST_CASE("Empty struct body") {
    helpers::test_parser_fail("struct {};",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_STRUCT, 0, 0});
}

TEST_CASE("Packed keyword out of order") {
    helpers::test_parser_fail(
        "struct packed { var a: Foo = bar; };",
        syntax::ParserDiagnostic{syntax::ParserError::PACKED_AFTER_STRUCT_KEYWORD, 0, 0},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{0uz, 34uz}});
}

} // namespace porpoise::tests

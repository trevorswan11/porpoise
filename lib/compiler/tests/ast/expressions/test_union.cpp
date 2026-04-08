#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Correct union") {
    helpers::test_expr_stmt(
        "union { a: i32, b: &mut T, };",
        ast::UnionExpression{
            syntax::Token{keywords::UNION},
            helpers::make_vector<ast::UnionField>(
                ast::UnionField{helpers::make_ident("a"),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}},
                ast::UnionField{helpers::make_ident("b"),
                                ast::ExplicitType{mods::MUT_REF, helpers::make_ident("T")}}),
            helpers::make_decls()});
}

TEST_CASE("Complex union fields") {
    helpers::test_expr_stmt(
        "union { a: *struct { var b: Foo = bar; }, b: enum : u64 {RED = 3u, B, }, };",
        ast::UnionExpression{
            syntax::Token{keywords::UNION},
            helpers::make_vector<ast::UnionField>(
                ast::UnionField{
                    helpers::make_ident("a"),
                    ast::ExplicitType{mods::PTR,
                                      mem::make_box<ast::StructExpression>(
                                          syntax::Token{keywords::STRUCT},
                                          helpers::make_decls(ast::DeclStatement{
                                              syntax::Token{keywords::VAR},
                                              helpers::make_ident("b"),
                                              mem::make_box<ast::TypeExpression>(
                                                  syntax::Token{syntax::TokenType::COLON, ":"},
                                                  ast::ExplicitType{
                                                      mods::BASE,
                                                      helpers::make_ident("Foo"),
                                                  }),
                                              helpers::make_ident("bar"),
                                              ast::DeclModifiers::VARIABLE,
                                          }))}},
                ast::UnionField{
                    helpers::make_ident("b"),
                    ast::ExplicitType{
                        mods::BASE,
                        mem::make_box<ast::EnumExpression>(
                            syntax::Token{keywords::ENUM},
                            helpers::make_ident("u64"),
                            helpers::make_vector<ast::Enumeration>(
                                ast::Enumeration{helpers::make_ident("RED"),
                                                 helpers::make_number<ast::U32Expression>("3u")},
                                ast::Enumeration{helpers::make_ident("B"), {}}),
                            helpers::make_decls())}}),
            helpers::make_decls()});
}

TEST_CASE("Union with decls") {
    const auto test = [](std::string_view input) {
        helpers::test_expr_stmt(
            input,
            ast::UnionExpression{
                syntax::Token{keywords::UNION},
                helpers::make_vector<ast::UnionField>(ast::UnionField{
                    helpers::make_ident("a"),
                    ast::ExplicitType{mods::PTR,
                                      mem::make_box<ast::StructExpression>(
                                          syntax::Token{keywords::STRUCT},
                                          helpers::make_decls(ast::DeclStatement{
                                              syntax::Token{keywords::VAR},
                                              helpers::make_ident("b"),
                                              mem::make_box<ast::TypeExpression>(
                                                  syntax::Token{syntax::TokenType::COLON, ":"},
                                                  ast::ExplicitType{
                                                      mods::BASE,
                                                      helpers::make_ident("Foo"),
                                                  }),
                                              helpers::make_ident("bar"),
                                              ast::DeclModifiers::VARIABLE,
                                          }))}}),
                helpers::make_decls(
                    ast::DeclStatement{
                        syntax::Token{keywords::CONST},
                        helpers::make_ident("b"),
                        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS},
                                                           std::nullopt),
                        mem::make_box<ast::FunctionExpression>(
                            syntax::Token{keywords::FN},
                            ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                            helpers::make_parameters(ast::FunctionParameter{
                                helpers::make_ident("a"), {mods::BASE, helpers::make_ident("A")}}),
                            false,
                            ast::ExplicitType{mods::BASE, helpers::make_ident("C")},
                            helpers::make_expr_block_stmt(helpers::ident_from("c"))),
                        ast::DeclModifiers::CONSTANT,
                    },
                    ast::DeclStatement{
                        syntax::Token{keywords::STATIC},
                        helpers::make_ident("a"),
                        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS},
                                                           std::nullopt),
                        helpers::make_number<ast::I32Expression>("2"),
                        ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
                    })});
    };

    constexpr std::string_view format_str =
        "union {{ a: *struct {{ var b: Foo = bar; }}{} const b := fn(&self, a: A): C {{ c; }}; "
        "static const a := 2; }};";
    test(fmt::format(format_str, ""));
    test(fmt::format(format_str, ","));
}

TEST_CASE("Illegal union field name") {
    helpers::test_parser_fail(
        "union { 2: i32 };",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 1, 9});
}

TEST_CASE("Illegal union field type") {
    helpers::test_parser_fail(
        "union { a: 2 };",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_EXPLICIT_TYPE, 1, 10});
}

TEST_CASE("Empty union") {
    helpers::test_parser_fail("union { };",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_UNION, 1, 1});
}

TEST_CASE("Empty union with decl") {
    helpers::test_parser_fail("union { const b := fn(&self, a: A): C { c; }; };",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_UNION, 1, 1});
}

TEST_CASE("Out of order union") {
    helpers::test_parser_fail("union { a: i32, const b := fn(&self, a: A): C { c; }; b: i32, };",
                              syntax::ParserDiagnostic{"Expected token SEMICOLON, found COLON",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 56uz}});
}

TEST_CASE("Non-static non-function union member") {
    helpers::test_parser_fail("union { a: i32, const b := 2; };",
                              syntax::ParserDiagnostic{syntax::ParserError::INVALID_MEMBER, 1, 17});
}

} // namespace porpoise::tests

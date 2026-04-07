#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

TEST_CASE("Correct union") {
    helpers::test_expr_stmt(
        "union { a: i32, b: &mut T, };",
        ast::UnionExpression{
            syntax::Token{keywords::UNION},
            helpers::make_vector<ast::UnionField>(
                ast::UnionField{helpers::make_ident("a"),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}},
                ast::UnionField{helpers::make_ident("b"),
                                ast::ExplicitType{mods::MUT_REF, helpers::make_ident("T")}})});
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
                                ast::Enumeration{
                                    helpers::make_ident("RED"),
                                    mem::make_box<ast::U32Expression>(
                                        syntax::Token{syntax::TokenType::UINT_10, "3u"}, 3u)},
                                ast::Enumeration{helpers::make_ident("B"), {}}))}})});
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

} // namespace porpoise::tests

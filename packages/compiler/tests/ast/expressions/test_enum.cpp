#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/primitive.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Basic enums") {
    helpers::test_expr_stmt(
        "enum {A, B, C};",
        ast::EnumExpression{syntax::Token{keywords::ENUM},
                            {},
                            helpers::make_vector<ast::Enumeration>(
                                ast::Enumeration{helpers::make_ident("A"), {}},
                                ast::Enumeration{helpers::make_ident("B"), {}},
                                ast::Enumeration{helpers::make_ident("C"), {}})});

    helpers::test_expr_stmt(
        "enum {A = 1, B = T, };",
        ast::EnumExpression{
            syntax::Token{keywords::ENUM},
            {},
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{helpers::make_ident("A"),
                                 mem::make_box<ast::SignedIntegerExpression>(
                                     syntax::Token{syntax::TokenType::INT_10, "1"}, 1)},
                ast::Enumeration{helpers::make_ident("B"), helpers::make_ident("T")})});
}

TEST_CASE("Underlying type") {
    helpers::test_expr_stmt(
        "enum : ulong {RED = 3u, B, };",
        ast::EnumExpression{
            syntax::Token{keywords::ENUM},
            helpers::make_ident("ulong"),
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{helpers::make_ident("RED"),
                                 mem::make_box<ast::UnsignedIntegerExpression>(
                                     syntax::Token{syntax::TokenType::UINT_10, "3u"}, 3u)},
                ast::Enumeration{helpers::make_ident("B"), {}})});

    helpers::test_expr_stmt(
        "enum : U {A = 0x4uz};",
        ast::EnumExpression{syntax::Token{keywords::ENUM},
                            helpers::make_ident("U"),
                            helpers::make_vector<ast::Enumeration>(ast::Enumeration{
                                helpers::make_ident("A"),
                                mem::make_box<ast::USizeIntegerExpression>(
                                    syntax::Token{syntax::TokenType::UZINT_16, "0x4uz"}, 0x4uz)})});
}

TEST_CASE("Empty enum") {
    helpers::test_parser_fail(
        "enum {};", syntax::ParserDiagnostic{syntax::ParserError::ENUM_MISSING_VARIANTS, 1, 6});
    helpers::test_parser_fail(
        "enum : T {};",
        syntax::ParserDiagnostic{syntax::ParserError::ENUM_MISSING_VARIANTS, 1, 10});
}

TEST_CASE("Illegal underlying type") {
    helpers::test_parser_fail(
        "enum : 4 {A};", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 8});
    helpers::test_parser_fail(
        R"(enum : "e" {A};)",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 8});
}

} // namespace porpoise::tests

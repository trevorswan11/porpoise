#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "lexer/keywords.hpp"
#include "parser/parser.hpp"

namespace conch::tests {

TEST_CASE("Basic enums") {
    helpers::test_expr_stmt(
        "enum {A, B, C};",
        ast::EnumExpression{
            Token{keywords::ENUM},
            {},
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "A"}),
                                 {}},
                ast::Enumeration{make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "B"}),
                                 {}},
                ast::Enumeration{make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "C"}),
                                 {}})});

    helpers::test_expr_stmt(
        "enum {A = 1, B = T, };",
        ast::EnumExpression{
            Token{keywords::ENUM},
            {},
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "A"}),
                    make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "1"}, 1)},
                ast::Enumeration{
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "B"}),
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "T"})})});
}

TEST_CASE("Underlying type") {
    helpers::test_expr_stmt(
        "enum : ulong {RED = 3u, B, };",
        ast::EnumExpression{
            Token{keywords::ENUM},
            make_box<ast::IdentifierExpression>(Token{TokenType::ULONG_TYPE, "ulong"}),
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{
                    make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "RED"}),
                    make_box<ast::UnsignedIntegerExpression>(Token{TokenType::UINT_10, "3u"}, 3u)},
                ast::Enumeration{make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "B"}),
                                 {}})});

    helpers::test_expr_stmt(
        "enum : U {A = 0x4uz};",
        ast::EnumExpression{Token{keywords::ENUM},
                            make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "U"}),
                            helpers::make_vector<ast::Enumeration>(ast::Enumeration{
                                make_box<ast::IdentifierExpression>(Token{TokenType::IDENT, "A"}),
                                make_box<ast::USizeIntegerExpression>(
                                    Token{TokenType::UZINT_16, "0x4uz"}, 0x4uz)})});
}

TEST_CASE("Empty enum") {
    helpers::test_fail("enum {};", ParserDiagnostic{ParserError::ENUM_MISSING_VARIANTS, 1, 1});
    helpers::test_fail("enum : T {};", ParserDiagnostic{ParserError::ENUM_MISSING_VARIANTS, 1, 1});
}

TEST_CASE("Illegal underlying type") {
    helpers::test_fail("enum : 4 {A};", ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 8});
    helpers::test_fail(R"(enum : "e" {A};)",
                       ParserDiagnostic{ParserError::ILLEGAL_IDENTIFIER, 1, 8});
}

} // namespace conch::tests

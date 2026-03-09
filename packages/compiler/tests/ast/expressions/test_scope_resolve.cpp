#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/scope_resolve.hpp"

namespace conch::tests {

TEST_CASE("Basic scope") {
    const Token a{TokenType::IDENT, "A"};
    helpers::test_expr_stmt(
        "A::B;",
        ast::ScopeResolutionExpression{a, helpers::make_ident(a), helpers::make_ident("B")});
}

TEST_CASE("Nested scope") {
    const Token a{TokenType::IDENT, "A"};
    helpers::test_expr_stmt(
        "A::B::C;",
        ast::ScopeResolutionExpression{a,
                                       make_box<ast::ScopeResolutionExpression>(
                                           a, helpers::make_ident(a), helpers::make_ident("B")),
                                       helpers::make_ident("C")});
}

TEST_CASE("Missing inner scope") {
    helpers::test_fail(
        "A:: ;",
        ParserDiagnostic{
            "Expected token IDENT, found SEMICOLON", ParserError::UNEXPECTED_TOKEN, 1, 5});
}

TEST_CASE("Illegal inner scope") {
    helpers::test_fail(
        "A::2;",
        ParserDiagnostic{
            "Expected token IDENT, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 4});
}

TEST_CASE("Illegal outer scope") {
    helpers::test_fail("2::A;", ParserDiagnostic{ParserError::ILLEGAL_OUTER_SCOPE_TYPE, 1, 1});
}

} // namespace conch::tests

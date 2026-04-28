#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

TEST_CASE("Basic scope") {
    const syntax::Token a{syntax::TokenType::IDENT, "A"};
    helpers::test_expr_stmt(
        "A::B;",
        ast::ScopeResolutionExpression{a, helpers::make_ident(a), helpers::make_ident("B")});
}

TEST_CASE("Nested scope") {
    const syntax::Token a{syntax::TokenType::IDENT, "A"};
    helpers::test_expr_stmt(
        "A::B::C;",
        ast::ScopeResolutionExpression{a,
                                       mem::make_box<ast::ScopeResolutionExpression>(
                                           a, helpers::make_ident(a), helpers::make_ident("B")),
                                       helpers::make_ident("C")});
}

TEST_CASE("Missing inner scope") {
    helpers::test_parser_fail(
        "A:: ;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found SEMICOLON", syntax::ParserError::UNEXPECTED_TOKEN, 0, 4});
}

TEST_CASE("Illegal inner scope") {
    helpers::test_parser_fail(
        "A::2;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 0, 3});
}

TEST_CASE("Illegal outer scope") {
    helpers::test_parser_fail(
        "2::A;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_OUTER_SCOPE_TYPE, 0, 0});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace operators = syntax::operators;

TEST_CASE("Unary expressions") {
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::BANG);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::NOT);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::MINUS);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::PLUS);
}

TEST_CASE("Dereference expression") {
    helpers::test_prefix_expr<ast::DereferenceExpression>(operators::STAR);
}

TEST_CASE("Reference expressions") {
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::BW_AND);
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::AND_MUT);
}

TEST_CASE("Prefix without operand") {
    helpers::test_parser_fail(
        "!", syntax::ParserDiagnostic{syntax::ParserError::PREFIX_MISSING_OPERAND, 1, 1});
    helpers::test_parser_fail(
        "!;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 2uz}});
}

} // namespace porpoise::tests

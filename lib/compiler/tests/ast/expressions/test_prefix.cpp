#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace operators = syntax::operators;

namespace helpers {

template <ast::LeafNode N> auto test_prefix_expr(const syntax::Operator& op) -> void {
    const auto input = fmt::format("{}a;", op.first);
    test_expr_stmt(input, N{syntax::Token{op}, make_ident("a")});
}

} // namespace helpers

TEST_CASE("Unary expressions") {
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::BANG);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::NOT);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::MINUS);
    helpers::test_prefix_expr<ast::UnaryExpression>(operators::PLUS);
}

TEST_CASE("Dereference expression") {
    helpers::test_prefix_expr<ast::DereferenceExpression>(operators::STAR);
}

TEST_CASE("Implicit access expression") {
    helpers::test_prefix_expr<ast::ImplicitAccessExpression>(operators::DOT);
}

TEST_CASE("Illegal implicit access operand") {
    helpers::test_parser_fail(
        ".a::b",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IMPLICIT_ACCESS_OPERAND, 0, 1});
}

TEST_CASE("Reference expressions") {
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::BW_AND);
    helpers::test_prefix_expr<ast::ReferenceExpression>(operators::AND_MUT);
}

TEST_CASE("Prefix without operand") {
    helpers::test_parser_fail(
        ".", syntax::ParserDiagnostic{syntax::ParserError::PREFIX_MISSING_OPERAND, 0, 0});

    helpers::test_parser_fail(
        "!;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{0uz, 1uz}});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/index.hpp"
#include "ast/expressions/primitive.hpp"

namespace porpoise::tests {

TEST_CASE("Single-level index") {
    const Token arr{TokenType::IDENT, "arr"};
    helpers::test_expr_stmt("arr[3uz];",
                            ast::IndexExpression{arr,
                                                 helpers::make_ident(arr),
                                                 make_box<ast::USizeIntegerExpression>(
                                                     Token{TokenType::UZINT_10, "3uz"}, 3uz)});

    helpers::test_expr_stmt(
        "arr[i];", ast::IndexExpression{arr, helpers::make_ident(arr), helpers::make_ident("i")});
}

TEST_CASE("Index on an index") {
    const Token arr{TokenType::IDENT, "arr"};
    helpers::test_expr_stmt(
        "arr[i][j];",
        ast::IndexExpression{
            arr,
            make_box<ast::IndexExpression>(arr, helpers::make_ident(arr), helpers::make_ident("i")),
            helpers::make_ident("j")});
}

TEST_CASE("No index") {
    helpers::test_parser_fail("arr[]",
                              ParserDiagnostic{ParserError::INDEX_MISSING_EXPRESSION, 1, 1});
}

} // namespace porpoise::tests

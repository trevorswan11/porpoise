#include <catch2/catch_test_macros.hpp>

#include "ast/expressions/primitive.hpp"
#include "helpers/ast.hpp"

#include "ast/statements/defer.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Correct defers") {
    const syntax::Token defer{keywords::DEFER};
    helpers::test_stmt("defer 3;",
                       ast::DeferStatement{defer,
                                           helpers::make_expr_stmt(ast::SignedIntegerExpression{
                                               syntax::Token{syntax::TokenType::INT_10, "3"}, 3})});

    helpers::test_stmt(
        "defer { a; };",
        ast::DeferStatement{defer, helpers::make_expr_block_stmt(helpers::ident_from("a"))});
}

TEST_CASE("Illegal deferred statements") {
    helpers::test_parser_fail(
        "defer import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_parser_fail(
        "defer return 3;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_parser_fail(
        "defer var a: int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_parser_fail(
        "defer using a = int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
}

TEST_CASE("Missing deferred statements") {
    helpers::test_parser_fail(
        "defer", syntax::ParserDiagnostic{syntax::ParserError::DEFER_MISSING_DEFERREE, 1, 1});
    helpers::test_parser_fail(
        "defer;", syntax::ParserDiagnostic{syntax::ParserError::DEFER_MISSING_DEFERREE, 1, 1});
}

} // namespace porpoise::tests

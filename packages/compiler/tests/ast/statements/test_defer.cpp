#include <catch2/catch_test_macros.hpp>

#include "ast/expressions/primitive.hpp"
#include "ast/helpers.hpp"

#include "ast/statements/defer.hpp"

namespace conch::tests {

TEST_CASE("Correct defers") {
    const Token defer{keywords::DEFER};
    helpers::test_stmt("defer 3;",
                       ast::DeferStatement{defer,
                                           helpers::make_expr_stmt(ast::SignedIntegerExpression{
                                               Token{TokenType::INT_10, "3"}, 3})});

    helpers::test_stmt(
        "defer { a; };",
        ast::DeferStatement{defer, helpers::make_expr_block_stmt(helpers::ident_from("a"))});
}

TEST_CASE("Illegal deferred statements") {
    helpers::test_fail("defer import std;",
                       ParserDiagnostic{ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_fail("defer return 3;",
                       ParserDiagnostic{ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_fail("defer var a: int;",
                       ParserDiagnostic{ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
    helpers::test_fail("defer using a = int;",
                       ParserDiagnostic{ParserError::ILLEGAL_DEFERRED_STATEMENT, 1, 7});
}

TEST_CASE("Missing deferred statements") {
    helpers::test_fail("defer", ParserDiagnostic{ParserError::DEFER_MISSING_DEFERREE, 1, 1});
    helpers::test_fail("defer;", ParserDiagnostic{ParserError::DEFER_MISSING_DEFERREE, 1, 1});
}

} // namespace conch::tests

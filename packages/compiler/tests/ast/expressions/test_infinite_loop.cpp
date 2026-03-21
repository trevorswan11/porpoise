#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/infinite_loop.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Correct infinite loop") {
    helpers::test_expr_stmt(
        "loop { a; };",
        ast::InfiniteLoopExpression{syntax::Token{keywords::LOOP},
                                    helpers::make_expr_block_stmt(helpers::ident_from("a"))});
}

TEST_CASE("Empty infinite loop") {
    helpers::test_parser_fail("loop {};",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_LOOP, 1, 6});
}

} // namespace porpoise::tests

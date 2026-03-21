#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/do_while.hpp"
#include "ast/expressions/primitive.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Correct do-while") {
    helpers::test_expr_stmt(
        "do {a;} while (true);",
        ast::DoWhileLoopExpression{
            syntax::Token{keywords::DO},
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true)});
}

TEST_CASE("Empty do-while") {
    helpers::test_parser_fail("do {} while (true);",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_LOOP, 1, 4});
}

TEST_CASE("Missing do-while condition") {
    helpers::test_parser_fail(
        "do {a; } while ();",
        syntax::ParserDiagnostic{syntax::ParserError::WHILE_MISSING_CONDITION, 1, 16});
}

TEST_CASE("Unclosed do-while body") {
    helpers::test_parser_fail("do { while (true);",
                              syntax::ParserDiagnostic{"Expected token LBRACE, found SEMICOLON",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 18uz}});
}

TEST_CASE("Unclosed do-while condition") {
    helpers::test_parser_fail("do {a; } while (true;",
                              syntax::ParserDiagnostic{"Expected token RPAREN, found SEMICOLON",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 21uz}});
}

} // namespace porpoise::tests

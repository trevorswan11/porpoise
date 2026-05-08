#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Correct do-while") {
    helpers::test_expr_stmt(
        "do {a;} while (true);",
        ast::DoWhileLoopExpression{syntax::Token{keywords::DO},
                                   helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                   helpers::make_primitive<ast::BoolExpression>(true)});
}

TEST_CASE("Empty do-while") {
    helpers::test_parser_fail("do {} while (true);",
                              syntax::Diagnostic{syntax::Error::EMPTY_LOOP, 0, 3});
}

TEST_CASE("Missing do-while condition") {
    helpers::test_parser_fail("do {a; } while ();",
                              syntax::Diagnostic{syntax::Error::WHILE_MISSING_CONDITION, 0, 15});
}

TEST_CASE("Unclosed do-while body") {
    helpers::test_parser_fail("do { while (true);",
                              syntax::Diagnostic{"Expected token LBRACE, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 17uz}});
}

TEST_CASE("Unclosed do-while condition") {
    helpers::test_parser_fail("do {a; } while (true;",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 20uz}});
}

} // namespace porpoise::tests

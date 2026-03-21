#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/while.hpp"
#include "ast/statements/jump.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("While without continuation or else") {
    helpers::test_expr_stmt(
        "while (true) {a;};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 {}});
}

TEST_CASE("While with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true),
                                 make_box<ast::AssignmentExpression>(
                                     syntax::Token{syntax::TokenType::IDENT, "i"},
                                     helpers::make_ident("i"),
                                     syntax::TokenType::PLUS_ASSIGN,
                                     make_box<ast::SignedIntegerExpression>(
                                         syntax::Token{syntax::TokenType::INT_10, "1"}, 1)),
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 {}});
}

TEST_CASE("While with else") {
    helpers::test_expr_stmt(
        "while (true) {a;} else return b;",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 make_box<ast::JumpStatement>(syntax::Token{keywords::RETURN},
                                                              helpers::make_ident("b"))});
}

TEST_CASE("Full while loop") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;} else return b;",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true),
                                 make_box<ast::AssignmentExpression>(
                                     syntax::Token{syntax::TokenType::IDENT, "i"},
                                     helpers::make_ident("i"),
                                     syntax::TokenType::PLUS_ASSIGN,
                                     make_box<ast::SignedIntegerExpression>(
                                         syntax::Token{syntax::TokenType::INT_10, "1"}, 1)),
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 make_box<ast::JumpStatement>(syntax::Token{keywords::RETURN},
                                                              helpers::make_ident("b"))});
}

TEST_CASE("Empty while with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 make_box<ast::BoolExpression>(syntax::Token{keywords::TRUE}, true),
                                 make_box<ast::AssignmentExpression>(
                                     syntax::Token{syntax::TokenType::IDENT, "i"},
                                     helpers::make_ident("i"),
                                     syntax::TokenType::PLUS_ASSIGN,
                                     make_box<ast::SignedIntegerExpression>(
                                         syntax::Token{syntax::TokenType::INT_10, "1"}, 1)),
                                 helpers::make_block_stmt(),
                                 {}});
}

TEST_CASE("Empty while") {
    helpers::test_parser_fail(
        "while (true) {};", syntax::ParserDiagnostic{syntax::ParserError::EMPTY_WHILE_LOOP, 1, 14});
}

TEST_CASE("Missing while condition") {
    helpers::test_parser_fail(
        "while () {a;};",
        syntax::ParserDiagnostic{syntax::ParserError::WHILE_MISSING_CONDITION, 1, 1},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 13uz}});
}

TEST_CASE("Unclosed while body") {
    helpers::test_parser_fail(
        "while (true) {;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 15uz}});
}

TEST_CASE("Unclosed while condition") {
    helpers::test_parser_fail(
        "while (true {};",
        syntax::ParserDiagnostic{
            "Expected token RPAREN, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 13});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_parser_fail(
        "while (true) : () {};",
        syntax::ParserDiagnostic{syntax::ParserError::EMPTY_WHILE_CONTINUATION, 1, 12});

    helpers::test_parser_fail(
        "while (true) : (i += 1 {};",
        syntax::ParserDiagnostic{
            "Expected token RPAREN, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 24});
}

TEST_CASE("Illegal while-else clause") {
    helpers::test_parser_fail(
        "while (true) : (i += 1) {} else import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LOOP_NON_BREAK, 1, 33});

    helpers::test_parser_fail(
        "while (true) : (i += 1) {} else;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 32uz}});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("While without continuation or else") {
    helpers::test_expr_stmt(
        "while (true) {a;};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 helpers::make_primitive<ast::BoolExpression>(true),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 {}});
}

TEST_CASE("While with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 helpers::make_primitive<ast::BoolExpression>(true),
                                 mem::make_nullable_box<ast::AssignmentExpression>(
                                     syntax::Token{syntax::TokenType::IDENT, "i"},
                                     helpers::make_ident("i"),
                                     syntax::TokenType::PLUS_ASSIGN,
                                     helpers::make_primitive<ast::I32Expression>("1")),
                                 helpers::make_expr_block_stmt(helpers::ident_from("a")),
                                 {}});
}

TEST_CASE("While with else") {
    helpers::test_expr_stmt(
        "while (true) {a;} else return b;",
        ast::WhileLoopExpression{
            syntax::Token{keywords::WHILE},
            helpers::make_primitive<ast::BoolExpression>(true),
            {},
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            mem::make_nullable_box<ast::ReturnStatement>(syntax::Token{keywords::RETURN},
                                                         helpers::make_ident<true>("b"))});
}

TEST_CASE("Full while loop") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {a;} else return b;",
        ast::WhileLoopExpression{
            syntax::Token{keywords::WHILE},
            helpers::make_primitive<ast::BoolExpression>(true),
            mem::make_nullable_box<ast::AssignmentExpression>(
                syntax::Token{syntax::TokenType::IDENT, "i"},
                helpers::make_ident("i"),
                syntax::TokenType::PLUS_ASSIGN,
                helpers::make_primitive<ast::I32Expression>("1")),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            mem::make_nullable_box<ast::ReturnStatement>(syntax::Token{keywords::RETURN},
                                                         helpers::make_ident<true>("b"))});
}

TEST_CASE("Empty while with continuation") {
    helpers::test_expr_stmt(
        "while (true) : (i += 1) {};",
        ast::WhileLoopExpression{syntax::Token{keywords::WHILE},
                                 helpers::make_primitive<ast::BoolExpression>(true),
                                 mem::make_nullable_box<ast::AssignmentExpression>(
                                     syntax::Token{syntax::TokenType::IDENT, "i"},
                                     helpers::make_ident("i"),
                                     syntax::TokenType::PLUS_ASSIGN,
                                     helpers::make_primitive<ast::I32Expression>("1")),
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
    helpers::test_parser_fail("while (true {};",
                              syntax::ParserDiagnostic{"Expected token RPAREN, found SEMICOLON",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 15uz}});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_parser_fail(
        "while (true) : () {};",
        syntax::ParserDiagnostic{syntax::ParserError::EMPTY_WHILE_CONTINUATION, 1, 12});

    helpers::test_parser_fail("while (true) : (i += 1 {};",
                              syntax::ParserDiagnostic{"Expected token RPAREN, found SEMICOLON",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 26uz}});
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

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

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
    helpers::test_parser_fail("while (true) {};",
                              syntax::Diagnostic{syntax::Error::EMPTY_LOOP, 0, 13});
}

TEST_CASE("Missing while condition") {
    helpers::test_parser_fail("while () {a;};",
                              syntax::Diagnostic{syntax::Error::WHILE_MISSING_CONDITION, 0, 0},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 12uz}});
}

TEST_CASE("Unclosed while body") {
    helpers::test_parser_fail("while (true) {;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 14uz}});
}

TEST_CASE("Unclosed while condition") {
    helpers::test_parser_fail("while (true {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 14uz}});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_parser_fail("while (true) : () {};",
                              syntax::Diagnostic{syntax::Error::EMPTY_WHILE_CONTINUATION, 0, 11});

    helpers::test_parser_fail("while (true) : (i += 1 {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 25uz}});
}

TEST_CASE("Illegal while-else clause") {
    helpers::test_parser_fail("while (true) : (i += 1) {} else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LOOP_NON_BREAK, 0, 32});

    helpers::test_parser_fail("while (true) : (i += 1) {} else;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 31uz}});
}

} // namespace porpoise::tests

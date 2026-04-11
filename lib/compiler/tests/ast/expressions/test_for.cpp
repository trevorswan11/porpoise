#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("For loop with base captures") {
    helpers::test_expr_stmt(
        "for (arr) |i| { a; };",
        ast::ForLoopExpression{
            syntax::Token{keywords::FOR},
            helpers::make_vector<mem::Box<ast::Expression>>(helpers::make_ident("arr")),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            {}});
}

TEST_CASE("For loop with modified captures") {
    helpers::test_expr_stmt(
        "for (arr, l, p) |i, &mut j, _| { a; };",
        ast::ForLoopExpression{
            syntax::Token{keywords::FOR},
            helpers::make_vector<mem::Box<ast::Expression>>(
                helpers::make_ident("arr"), helpers::make_ident("l"), helpers::make_ident("p")),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")},
                ast::ForLoopCapture::Valued{ast::TypeModifier{ast::TypeModifier::Modifier::MUT_REF},
                                            helpers::make_ident("j")},
                ast::ForLoopCapture{syntax::Token{syntax::TokenType::UNDERSCORE, "_"}}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            {}});
}

TEST_CASE("Full for loop with else") {
    helpers::test_expr_stmt(
        "for (0..4) |i| { a; } else return b;",
        ast::ForLoopExpression{
            syntax::Token{keywords::FOR},
            helpers::make_vector<mem::Box<ast::Expression>>(mem::make_box<ast::RangeExpression>(
                syntax::Token{syntax::TokenType::INT_10, "0"},
                helpers::make_primitive<ast::I32Expression>("0"),
                syntax::TokenType::DOT_DOT,
                helpers::make_primitive<ast::I32Expression>("4"))),
            helpers::make_vector<ast::ForLoopCapture>(
                ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}),
            helpers::make_expr_block_stmt(helpers::ident_from("a")),
            mem::make_nullable_box<ast::JumpStatement>(syntax::Token{keywords::RETURN},
                                                       helpers::make_ident<true>("b"))});
}

TEST_CASE("Non-terminated iterables") {
    helpers::test_parser_fail(
        "for (0..4 |i| { a; } else return b;",
        syntax::ParserDiagnostic{"Expected token RBRACE, found IDENT",
                                 syntax::ParserError::UNEXPECTED_TOKEN,
                                 std::pair{1uz, 17uz}},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 20uz}});
}

TEST_CASE("Missing iterables") {
    helpers::test_parser_fail(
        "for () |i| { a; } else return b;",
        syntax::ParserDiagnostic{syntax::ParserError::FOR_MISSING_ITERABLES, 1, 1},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 17uz}});

    helpers::test_parser_fail(
        "for |i| { a; } else return b;",
        syntax::ParserDiagnostic{
            "Expected token LPAREN, found BW_OR", syntax::ParserError::UNEXPECTED_TOKEN, 1, 5},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 14uz}});
}

TEST_CASE("Non-terminated captures") {
    helpers::test_parser_fail(
        "for (0..4) |i { a; } else return b;",
        syntax::ParserDiagnostic{
            "Expected token COMMA, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 15},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 20uz}});
}

TEST_CASE("Missing captures") {
    helpers::test_parser_fail(
        "for (0..4) { a; } else return b;",
        syntax::ParserDiagnostic{
            "Expected token BW_OR, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 12},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 17uz}});
}

TEST_CASE("Illegal capture") {
    helpers::test_parser_fail(
        "for (0..4) |2| { a; } else return b;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 13},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 21uz}});
}

TEST_CASE("Iterable-capture mismatch") {
    helpers::test_parser_fail(
        "for (0..4) |i, j| { a; } else return b;",
        syntax::ParserDiagnostic{syntax::ParserError::FOR_ITERABLE_CAPTURE_MISMATCH, 1, 1});
}

TEST_CASE("Empty for block") {
    helpers::test_parser_fail("for (0..4) |i| {} else return b;",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_FOR_LOOP, 1, 16});
}

TEST_CASE("Illegal for-else clause") {
    helpers::test_parser_fail(
        "for (0..4) |i| { a; } else import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LOOP_NON_BREAK, 1, 28});
}

} // namespace porpoise::tests

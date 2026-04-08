#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace operators = syntax::operators;

TEST_CASE("Empty implicit initializer list") {
    helpers::test_expr_stmt(
        ".{};",
        syntax::Token{operators::DOT},
        ast::InitializerExpression{syntax::Token{syntax::TokenType::LBRACE, "{"},
                                   std::nullopt,
                                   helpers::make_vector<ast::Initializer>()});
}

TEST_CASE("Non-empty implicit initializer list") {
    helpers::test_expr_stmt(
        ".{ .a = 2, };",
        syntax::Token{operators::DOT},
        ast::InitializerExpression{syntax::Token{syntax::TokenType::LBRACE, "{"},
                                   std::nullopt,
                                   helpers::make_vector<ast::Initializer>(ast::Initializer{
                                       mem::make_box<ast::ImplicitAccessExpression>(
                                           syntax::Token{operators::DOT}, helpers::make_ident("a")),
                                       helpers::make_number<ast::I32Expression>("2")})});
}

TEST_CASE("Empty explicit initializer list") {
    helpers::test_expr_stmt("T{};",
                            ast::InitializerExpression{syntax::Token{syntax::TokenType::IDENT, "T"},
                                                       helpers::make_ident("T"),
                                                       helpers::make_vector<ast::Initializer>()});
}

TEST_CASE("Non-empty explicit initializer list") {
    helpers::test_expr_stmt(
        "T{ .a = 2 };",
        ast::InitializerExpression{syntax::Token{syntax::TokenType::IDENT, "T"},
                                   helpers::make_ident("T"),
                                   helpers::make_vector<ast::Initializer>(ast::Initializer{
                                       mem::make_box<ast::ImplicitAccessExpression>(
                                           syntax::Token{operators::DOT}, helpers::make_ident("a")),
                                       helpers::make_number<ast::I32Expression>("2")})});
}

TEST_CASE("Multiple initializer key-values") {
    helpers::test_expr_stmt(
        "T{ .a = 2, .b = 3u, .c = t };",
        ast::InitializerExpression{
            syntax::Token{syntax::TokenType::IDENT, "T"},
            helpers::make_ident("T"),
            helpers::make_vector<ast::Initializer>(
                ast::Initializer{mem::make_box<ast::ImplicitAccessExpression>(
                                     syntax::Token{operators::DOT}, helpers::make_ident("a")),
                                 helpers::make_number<ast::I32Expression>("2")},
                ast::Initializer{mem::make_box<ast::ImplicitAccessExpression>(
                                     syntax::Token{operators::DOT}, helpers::make_ident("b")),
                                 helpers::make_number<ast::U32Expression>("3u")},
                ast::Initializer{mem::make_box<ast::ImplicitAccessExpression>(
                                     syntax::Token{operators::DOT}, helpers::make_ident("c")),
                                 helpers::make_ident("t")})});
}

TEST_CASE("Unclosed implicit initializer") {
    helpers::test_parser_fail(".{",
                              syntax::ParserDiagnostic{"Expected token RBRACE, found END",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 3uz}});

    helpers::test_parser_fail(".{ .a = 2",
                              syntax::ParserDiagnostic{"Expected token COMMA, found END",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 10uz}});
}

TEST_CASE("Unclosed explicit initializer") {
    helpers::test_parser_fail("T{",
                              syntax::ParserDiagnostic{"Expected token RBRACE, found END",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 3uz}});

    helpers::test_parser_fail("T{ .a = 2",
                              syntax::ParserDiagnostic{"Expected token COMMA, found END",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 10uz}});
}

TEST_CASE("Malformed initializer key-value") {
    helpers::test_parser_fail(
        "T{ .a = };",
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 9uz}});
}

} // namespace porpoise::tests

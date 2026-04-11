#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Match without alternate") {
    helpers::test_expr_stmt(
        "match (a) { b => c; };",
        ast::MatchExpression{
            syntax::Token{keywords::MATCH},
            helpers::make_ident("a"),
            helpers::make_vector<ast::MatchArm>(ast::MatchArm{
                helpers::make_ident("b"), {}, helpers::make_expr_stmt(helpers::ident_from("c"))}),
            {}});

    helpers::test_expr_stmt(
        "match (a) { b => c; d => e; };",
        ast::MatchExpression{syntax::Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(
                                 ast::MatchArm{helpers::make_ident("b"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("c"))},
                                 ast::MatchArm{helpers::make_ident("d"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("e"))}),
                             {}});
}

TEST_CASE("Match with arm capture") {
    helpers::test_expr_stmt(
        "match (a) { b => |c| d; e => |_| f; g => h; };",
        ast::MatchExpression{syntax::Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(
                                 ast::MatchArm{helpers::make_ident("b"),
                                               helpers::make_ident("c"),
                                               helpers::make_expr_stmt(helpers::ident_from("d"))},
                                 ast::MatchArm{helpers::make_ident("e"),
                                               Unit{},
                                               helpers::make_expr_stmt(helpers::ident_from("f"))},
                                 ast::MatchArm{helpers::make_ident("g"),
                                               {},
                                               helpers::make_expr_stmt(helpers::ident_from("h"))}),
                             {}});
}

TEST_CASE("Match with alternate") {
    helpers::test_expr_stmt(
        "match (a) { b => { c; } } else d;",
        ast::MatchExpression{
            syntax::Token{keywords::MATCH},
            helpers::make_ident("a"),
            helpers::make_vector<ast::MatchArm>(
                ast::MatchArm{helpers::make_ident("b"),
                              {},
                              helpers::make_expr_block_stmt(helpers::ident_from("c"))}),
            helpers::make_expr_stmt<ast::IdentifierExpression, true>(helpers::ident_from("d"))});
}

TEST_CASE("Match without condition") {
    helpers::test_parser_fail(
        "match () { b => c; };",
        syntax::ParserDiagnostic{syntax::ParserError::MATCH_EXPR_MISSING_CONDITION, 1, 1},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 20uz}});

    helpers::test_parser_fail(
        "match { b => c; };",
        syntax::ParserDiagnostic{
            "Expected token LPAREN, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 17uz}});
}

TEST_CASE("Armless match expression") {
    helpers::test_parser_fail(
        "match (a) {};", syntax::ParserDiagnostic{syntax::ParserError::ARMLESS_MATCH_EXPR, 1, 1});
}

TEST_CASE("Malformed arm LHS") {
    helpers::test_parser_fail(
        "match {  => c; };",
        syntax::ParserDiagnostic{
            "Expected token LPAREN, found LBRACE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 16uz}});
}

TEST_CASE("Illegal match arm rhs") {
    helpers::test_parser_fail(
        "match (a) { b => import std; };",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_MATCH_ARM, 1, 18});
}

TEST_CASE("Arm missing fat arrow") {
    helpers::test_parser_fail(
        "match (a) { b c; };",
        syntax::ParserDiagnostic{
            "Expected token FAT_ARROW, found IDENT", syntax::ParserError::UNEXPECTED_TOKEN, 1, 15},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 18uz}});
}

TEST_CASE("Illegal match alternate") {
    helpers::test_parser_fail(
        "match (a) { b => c; } else import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_MATCH_CATCH_ALL, 1, 28});
}

} // namespace porpoise::tests

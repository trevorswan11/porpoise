#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

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
        ast::MatchExpression{syntax::Token{keywords::MATCH},
                             helpers::make_ident("a"),
                             helpers::make_vector<ast::MatchArm>(ast::MatchArm{
                                 helpers::make_ident("b"),
                                 {},
                                 helpers::make_expr_block_stmt(helpers::ident_from("c"))}),
                             helpers::make_expr_stmt<true>(helpers::ident_from("d"))});
}

TEST_CASE("Match without condition") {
    helpers::test_parser_fail("match () { b => c; };",
                              syntax::Diagnostic{syntax::Error::MATCH_EXPR_MISSING_CONDITION, 0, 0},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 19uz}});

    helpers::test_parser_fail(
        "match { b => c; };",
        syntax::Diagnostic{
            "Expected token LPAREN, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 6},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 16uz}});
}

TEST_CASE("Armless match expression") {
    helpers::test_parser_fail("match (a) {};",
                              syntax::Diagnostic{syntax::Error::ARMLESS_MATCH_EXPR, 0, 0});
}

TEST_CASE("Malformed arm LHS") {
    helpers::test_parser_fail(
        "match {  => c; };",
        syntax::Diagnostic{
            "Expected token LPAREN, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 6},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 15uz}});
}

TEST_CASE("Illegal match arm rhs") {
    helpers::test_parser_fail("match (a) { b => import std; };",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_MATCH_ARM, 0, 17});
}

TEST_CASE("Arm missing fat arrow") {
    helpers::test_parser_fail(
        "match (a) { b c; };",
        syntax::Diagnostic{
            "Expected token FAT_ARROW, found IDENT", syntax::Error::UNEXPECTED_TOKEN, 0, 14},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 17uz}});
}

TEST_CASE("Illegal match alternate") {
    helpers::test_parser_fail("match (a) { b => c; } else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_MATCH_CATCH_ALL, 0, 27});
}

} // namespace porpoise::tests

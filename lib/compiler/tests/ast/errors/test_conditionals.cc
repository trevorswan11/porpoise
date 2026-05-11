#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("If without condition") {
    helpers::test_parser_fail("if () b;",
                              syntax::Diagnostic{syntax::Error::IF_MISSING_CONDITION, 0, 0});
}

TEST_CASE("If with illegal consequence") {
    helpers::test_parser_fail("if (a) import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IF_BRANCH, 0, 7});
}

TEST_CASE("If with illegal alternate") {
    helpers::test_parser_fail("if (a) {} else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IF_BRANCH, 0, 15});
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

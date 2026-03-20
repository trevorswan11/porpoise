#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/statements/discard.hpp"

namespace porpoise::tests {

TEST_CASE("Discard statements") {
    const Token start_token{TokenType::UNDERSCORE, "_"};
    helpers::test_stmt(
        "_ = 4;",
        ast::DiscardStatement{
            start_token, make_box<ast::SignedIntegerExpression>(Token{TokenType::INT_10, "4"}, 4)});

    std::vector<ast::Enumeration> enumerations;
    enumerations.emplace_back(ast::Enumeration{helpers::make_ident("RED"), std::nullopt});
    helpers::test_stmt(
        "_ = enum { RED };",
        ast::DiscardStatement{start_token,
                              make_box<ast::EnumExpression>(
                                  Token{keywords::ENUM}, std::nullopt, std::move(enumerations))});
}

TEST_CASE("Malformed discardees") {
    SECTION("Misplaced correct statement") {
        helpers::test_parser_fail(
            "_ = import std;",
            ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                             ParserError::MISSING_PREFIX_PARSER,
                             1,
                             5});
    }

    SECTION("Misplaced incorrect statement") {
        helpers::test_parser_fail(
            "_ = import 3;",
            ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                             ParserError::MISSING_PREFIX_PARSER,
                             1,
                             5});
    }
}

TEST_CASE("Missing discardee") {
    helpers::test_parser_fail("_ = ",
                              ParserDiagnostic{ParserError::DISCARD_MISSING_DISCARDEE, 1, 3});
    helpers::test_parser_fail("_ = ;",
                              ParserDiagnostic{ParserError::DISCARD_MISSING_DISCARDEE, 1, 3});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/statements/discard.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Discard statements") {
    const syntax::Token start_token{syntax::TokenType::UNDERSCORE, "_"};
    helpers::test_stmt(
        "_ = 4;",
        ast::DiscardStatement{start_token,
                              make_box<ast::SignedIntegerExpression>(
                                  syntax::Token{syntax::TokenType::INT_10, "4"}, 4)});

    std::vector<ast::Enumeration> enumerations;
    enumerations.emplace_back(ast::Enumeration{helpers::make_ident("RED"), std::nullopt});
    helpers::test_stmt(
        "_ = enum { RED };",
        ast::DiscardStatement{start_token,
                              make_box<ast::EnumExpression>(syntax::Token{keywords::ENUM},
                                                            std::nullopt,
                                                            std::move(enumerations))});
}

TEST_CASE("Malformed discardees") {
    SECTION("Misplaced correct statement") {
        helpers::test_parser_fail(
            "_ = import std;",
            syntax::ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                                     syntax::ParserError::MISSING_PREFIX_PARSER,
                                     std::pair{1uz, 5uz}});
    }

    SECTION("Misplaced incorrect statement") {
        helpers::test_parser_fail(
            "_ = import 3;",
            syntax::ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                                     syntax::ParserError::MISSING_PREFIX_PARSER,
                                     std::pair{1uz, 5uz}});
    }
}

TEST_CASE("Missing discardee") {
    helpers::test_parser_fail(
        "_ = ", syntax::ParserDiagnostic{syntax::ParserError::DISCARD_MISSING_DISCARDEE, 1, 3});
    helpers::test_parser_fail(
        "_ = ;", syntax::ParserDiagnostic{syntax::ParserError::DISCARD_MISSING_DISCARDEE, 1, 3});
}

} // namespace porpoise::tests

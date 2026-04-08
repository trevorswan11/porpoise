#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Expressionless jumps") {
    helpers::test_stmt("continue;", ast::JumpStatement{syntax::Token{keywords::CONTINUE}, {}});
    helpers::test_stmt("break;", ast::JumpStatement{syntax::Token{keywords::BREAK}, {}});
    helpers::test_stmt("return;", ast::JumpStatement{syntax::Token{keywords::RETURN}, {}});
}

TEST_CASE("Expression returns") {
    helpers::test_stmt("return 4;",
                       ast::JumpStatement{syntax::Token{keywords::RETURN},
                                          helpers::make_primitive<ast::I32Expression>("4")});

    helpers::test_stmt(
        "return enum { RED };",
        ast::JumpStatement{syntax::Token{keywords::RETURN},
                           mem::make_box<ast::EnumExpression>(
                               syntax::Token{keywords::ENUM},
                               std::nullopt,
                               helpers::make_vector<ast::Enumeration>(
                                   ast::Enumeration{helpers::make_ident("RED"), std::nullopt}),
                               helpers::make_decls())});
}

TEST_CASE("Incorrectly terminated jumps") {
    const auto inputs = std::to_array<std::string_view>({"return", "continue", "break"});
    for (const auto& input : inputs) {
        helpers::test_parser_fail(input,
                                  syntax::ParserDiagnostic{"Expected token SEMICOLON, found END",
                                                           syntax::ParserError::UNEXPECTED_TOKEN,
                                                           std::pair{1uz, input.size() + 1}});
    }

    helpers::test_parser_fail(
        "return return",
        syntax::ParserDiagnostic{"No prefix parse function for RETURN(return) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 8uz}});
}

TEST_CASE("Illegal control flow") {
    helpers::test_parser_fail("continue 4;",
                              syntax::ParserDiagnostic{"Expected token SEMICOLON, found INT_10",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 10uz}});

    helpers::test_parser_fail(
        "break 4;",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7});
}

} // namespace porpoise::tests

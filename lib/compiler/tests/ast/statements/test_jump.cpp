#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Expressionless jumps") {
    helpers::test_stmt("return;", ast::ReturnStatement{syntax::Token{keywords::RETURN}, {}});
    helpers::test_stmt("continue;", ast::ContinueStatement{syntax::Token{keywords::CONTINUE}, {}});
    helpers::test_stmt("break;", ast::BreakStatement{syntax::Token{keywords::BREAK}, {}, {}});
}

TEST_CASE("Labeled breaks") {
    helpers::test_stmt(
        "break :blk;",
        ast::BreakStatement{syntax::Token{keywords::BREAK}, helpers::make_ident<true>("blk"), {}});

    helpers::test_stmt("break :blk 1;",
                       ast::BreakStatement{syntax::Token{keywords::BREAK},
                                           helpers::make_ident<true>("blk"),
                                           helpers::make_primitive<ast::I32Expression, true>("1")});
}

TEST_CASE("Labeled continues") {
    helpers::test_stmt("continue :blk;",
                       ast::ContinueStatement{syntax::Token{keywords::CONTINUE},
                                              helpers::make_ident<true>("blk")});
}

TEST_CASE("Expression returns") {
    helpers::test_stmt(
        "return 4;",
        ast::ReturnStatement{syntax::Token{keywords::RETURN},
                             helpers::make_primitive<ast::I32Expression, true>("4")});

    helpers::test_stmt(
        "return enum { RED };",
        ast::ReturnStatement{syntax::Token{keywords::RETURN},
                             mem::make_nullable_box<ast::EnumExpression>(
                                 syntax::Token{keywords::ENUM},
                                 nullptr,
                                 helpers::make_vector<ast::Enumeration>(
                                     ast::Enumeration{helpers::make_ident("RED"), {}}),
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

TEST_CASE("Illegal continue/break control flow") {
    helpers::test_parser_fail("continue 4;",
                              syntax::ParserDiagnostic{syntax::ParserError::VALUED_CONTINUE, 1, 1});

    helpers::test_parser_fail(
        "break 4;",
        syntax::ParserDiagnostic{syntax::ParserError::VALUED_BREAK_MISSING_LABEL, 1, 1});
}

} // namespace porpoise::tests

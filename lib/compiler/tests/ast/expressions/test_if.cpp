#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("If without alternate") {
    helpers::test_expr_stmt(
        "if (a) { b; };",
        ast::IfExpression{syntax::Token{keywords::IF},
                          false,
                          helpers::make_ident("a"),
                          helpers::make_expr_block_stmt(helpers::ident_from("b")),
                          {}});

    helpers::test_expr_stmt("if constexpr (a) b;",
                            ast::IfExpression{syntax::Token{keywords::IF},
                                              true,
                                              helpers::make_ident("a"),
                                              helpers::make_expr_stmt(helpers::ident_from("b")),
                                              {}});
}

TEST_CASE("If with alternate") {
    helpers::test_expr_stmt(
        "if (a) { b; } else { c; };",
        ast::IfExpression{syntax::Token{keywords::IF},
                          false,
                          helpers::make_ident("a"),
                          helpers::make_expr_block_stmt(helpers::ident_from("b")),
                          helpers::make_expr_block_stmt(helpers::ident_from("c"))});

    helpers::test_expr_stmt(
        "if (a) b; else { c; };",
        ast::IfExpression{syntax::Token{keywords::IF},
                          false,
                          helpers::make_ident("a"),
                          helpers::make_expr_stmt(helpers::ident_from("b")),
                          helpers::make_expr_block_stmt(helpers::ident_from("c"))});
}

TEST_CASE("If without condition") {
    helpers::test_parser_fail(
        "if () b;", syntax::ParserDiagnostic{syntax::ParserError::IF_MISSING_CONDITION, 1, 1});
}

TEST_CASE("If with illegal consequence") {
    helpers::test_parser_fail(
        "if (a) import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IF_BRANCH, 1, 8});
}

TEST_CASE("If with illegal alternate") {
    helpers::test_parser_fail(
        "if (a) {} else import std;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IF_BRANCH, 1, 16});
}

} // namespace porpoise::tests

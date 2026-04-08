#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Well formed test statement") {
    helpers::test_stmt(
        R"(test "good" { a; b; c; })",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           helpers::make_primitive<ast::StringExpression>(R"("good")"),
                           helpers::make_expr_block_stmt(helpers::ident_from("a"),
                                                         helpers::ident_from("b"),
                                                         helpers::ident_from("c"))});
}

TEST_CASE("Forwarding test") {
    helpers::test_stmt(
        "test { import other; };",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           std::nullopt,
                           helpers::make_block_stmt(ast::ImportStatement{
                               syntax::Token{keywords::IMPORT},
                               ast::LibraryImport{helpers::make_ident("other"), {}}})});
}

TEST_CASE("Empty test") {
    helpers::test_stmt("test {};",
                       ast::TestStatement{syntax::Token{keywords::TEST},
                                          std::nullopt,
                                          helpers::make_block_stmt()});

    helpers::test_stmt(
        R"(test "something" {})",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           helpers::make_primitive<ast::StringExpression>(R"("something")"),
                           helpers::make_block_stmt()});
}

TEST_CASE("Non-terminated test") {
    helpers::test_parser_fail(
        "test {",
        syntax::ParserDiagnostic{
            "Expected token RBRACE, found END", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7});
}

TEST_CASE("Empty test description") {
    helpers::test_parser_fail(
        R"(test "" {};)",
        syntax::ParserDiagnostic{syntax::ParserError::EMPTY_TEST_DESCRIPTION, 1, 6});
}

} // namespace porpoise::tests

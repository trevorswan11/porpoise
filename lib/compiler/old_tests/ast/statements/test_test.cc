#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Well formed test statement") {
    helpers::test_stmt(
        R"(test "good" { a; b; c; })",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           helpers::make_primitive<ast::StringExpression, true>(R"("good")"),
                           helpers::make_expr_block_stmt(helpers::ident_from("a"),
                                                         helpers::ident_from("b"),
                                                         helpers::ident_from("c"))});
}

TEST_CASE("Forwarding test") {
    helpers::test_stmt(
        "test { import other; };",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           {},
                           helpers::make_block_stmt(ast::ImportStatement{
                               syntax::Token{keywords::IMPORT},
                               ast::LibraryImport{helpers::make_ident("other"), {}}})});
}

TEST_CASE("Empty test") {
    helpers::test_stmt(
        "test {};",
        ast::TestStatement{syntax::Token{keywords::TEST}, {}, helpers::make_block_stmt()});

    helpers::test_stmt(
        R"(test "something" {})",
        ast::TestStatement{syntax::Token{keywords::TEST},
                           helpers::make_primitive<ast::StringExpression, true>(R"("something")"),
                           helpers::make_block_stmt()});
}

TEST_CASE("Non-terminated test") {
    helpers::test_parser_fail(
        "test {",
        syntax::Diagnostic{
            "Expected token RBRACE, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Empty test description") {
    helpers::test_parser_fail(R"(test "" {};)",
                              syntax::Diagnostic{syntax::Error::EMPTY_TEST_DESCRIPTION, 0, 5});
}

} // namespace porpoise::tests

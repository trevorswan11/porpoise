#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("Array size token requirement") {
    helpers::test_parser_fail("[]i32{2};",
                              syntax::Diagnostic{syntax::Error::MISSING_ARRAY_SIZE_TOKEN, 0, 0});
}

TEST_CASE("No arguments with comma") {
    helpers::test_parser_fail(
        "func(,)", syntax::Diagnostic{syntax::Error::COMMA_WITH_MISSING_CALL_ARGUMENT, 0, 5});
}

TEST_CASE("Non-comma separated arguments") {
    helpers::test_parser_fail(
        "func(1 2)",
        syntax::Diagnostic{
            "Expected token COMMA, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Non-terminated identifier") {
    helpers::test_parser_fail(
        "foobar",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("No index") {
    helpers::test_parser_fail("arr[]",
                              syntax::Diagnostic{syntax::Error::INDEX_MISSING_EXPRESSION, 0, 3});
}

TEST_CASE("Illegal infix node") {
    helpers::test_parser_fail(
        "a and import std;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 6uz}});
}

TEST_CASE("Non-terminated infix") {
    helpers::test_parser_fail("a and;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 5uz}});
    helpers::test_parser_fail("a and", syntax::Diagnostic{syntax::Error::INFIX_MISSING_RHS, 0, 2});
}

TEST_CASE("Unclosed implicit initializer") {
    helpers::test_parser_fail(".{",
                              syntax::Diagnostic{"Expected token RBRACE, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 2uz}});

    helpers::test_parser_fail(".{ .a = 2",
                              syntax::Diagnostic{"Expected token COMMA, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 9uz}});
}

TEST_CASE("Unclosed explicit initializer") {
    helpers::test_parser_fail("T{",
                              syntax::Diagnostic{"Expected token RBRACE, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 2uz}});

    helpers::test_parser_fail("T{ .a = 2",
                              syntax::Diagnostic{"Expected token COMMA, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 9uz}});
}

TEST_CASE("Malformed initializer key-value") {
    helpers::test_parser_fail("T{ .a = };",
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 8uz}});
}

TEST_CASE("Non-ident label") {
    helpers::test_parser_fail("2: {};", syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL, 0, 1});
}

TEST_CASE("Illegal label expressions") {
    helpers::test_parser_fail("a: 3;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail("a: b;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail("a: b();",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail("a: b: c: {};",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_EXPRESSION, 0, 6});
}

TEST_CASE("Illegal label statements") {
    helpers::test_parser_fail("a: defer 3;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_STATEMENT, 0, 3});

    helpers::test_parser_fail("a: return 3;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LABEL_STATEMENT, 0, 3});
}

TEST_CASE("Illegal implicit access operand") {
    helpers::test_parser_fail(
        ".a::b", syntax::Diagnostic{syntax::Error::ILLEGAL_IMPLICIT_ACCESS_OPERAND, 0, 2});
}

TEST_CASE("Prefix without operand") {
    helpers::test_parser_fail(".", syntax::Diagnostic{syntax::Error::PREFIX_MISSING_OPERAND, 0, 0});

    helpers::test_parser_fail("!;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 1uz}});
}

TEST_CASE("Missing inner scope") {
    helpers::test_parser_fail(
        "A:: ;",
        syntax::Diagnostic{
            "Expected token IDENT, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 4});
}

TEST_CASE("Illegal inner scope") {
    helpers::test_parser_fail(
        "A::2;",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 3});
}

TEST_CASE("Illegal outer scope") {
    helpers::test_parser_fail("2::A;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_OUTER_SCOPE_TYPE, 0, 0});
}

} // namespace porpoise::tests

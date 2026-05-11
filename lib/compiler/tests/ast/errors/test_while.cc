#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("Empty do-while") {
    helpers::test_parser_fail("do {} while (true);",
                              syntax::Diagnostic{syntax::Error::EMPTY_LOOP, 0, 3});
}

TEST_CASE("Missing do-while condition") {
    helpers::test_parser_fail("do {a; } while ();",
                              syntax::Diagnostic{syntax::Error::WHILE_MISSING_CONDITION, 0, 15});
}

TEST_CASE("Unclosed do-while body") {
    helpers::test_parser_fail("do { while (true);",
                              syntax::Diagnostic{"Expected token LBRACE, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 17uz}});
}

TEST_CASE("Unclosed do-while condition") {
    helpers::test_parser_fail("do {a; } while (true;",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 20uz}});
}

TEST_CASE("Empty infinite loop") {
    helpers::test_parser_fail("loop {};", syntax::Diagnostic{syntax::Error::EMPTY_LOOP, 0, 5});
}

TEST_CASE("Empty while") {
    helpers::test_parser_fail("while (true) {};",
                              syntax::Diagnostic{syntax::Error::EMPTY_LOOP, 0, 13});
}

TEST_CASE("Missing while condition") {
    helpers::test_parser_fail("while () {a;};",
                              syntax::Diagnostic{syntax::Error::WHILE_MISSING_CONDITION, 0, 0},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 12uz}});
}

TEST_CASE("Unclosed while body") {
    helpers::test_parser_fail("while (true) {;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 14uz}});
}

TEST_CASE("Unclosed while condition") {
    helpers::test_parser_fail("while (true {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 14uz}});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_parser_fail("while (true) : () {};",
                              syntax::Diagnostic{syntax::Error::EMPTY_WHILE_CONTINUATION, 0, 11});

    helpers::test_parser_fail("while (true) : (i += 1 {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 25uz}});
}

TEST_CASE("Illegal while-else clause") {
    helpers::test_parser_fail("while (true) : (i += 1) {} else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LOOP_NON_BREAK, 0, 32});

    helpers::test_parser_fail("while (true) : (i += 1) {} else;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 31uz}});
}

} // namespace porpoise::tests

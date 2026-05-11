#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("Empty enum") {
    helpers::test_parser_fail("enum {};", syntax::Diagnostic{syntax::Error::EMPTY_ENUM, 0, 0});
    helpers::test_parser_fail("enum : T {};", syntax::Diagnostic{syntax::Error::EMPTY_ENUM, 0, 0});
}

TEST_CASE("Illegal underlying type") {
    helpers::test_parser_fail("enum : 4 {A};",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 7});
    helpers::test_parser_fail(R"(enum : "e" {A};)",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 7});
}

TEST_CASE("Empty enum with decl") {
    helpers::test_parser_fail("enum : i64 { const b := fn(&self, a: A): C { c; }; };",
                              syntax::Diagnostic{syntax::Error::EMPTY_ENUM, 0, 0});
}

TEST_CASE("Out of order enum") {
    helpers::test_parser_fail("enum : i64 { A = 2l const b := fn(&self, a: A): C { c; }; B = 2l };",
                              syntax::Diagnostic{"Expected token SEMICOLON, found RBRACE",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 65uz}});
}

TEST_CASE("Non-static non-function enum decl") {
    helpers::test_parser_fail("enum { A = i32{} const b := 2; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 17});
}

TEST_CASE("Illegal struct members") {
    helpers::test_parser_fail("struct { const foo := bar; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 9});

    helpers::test_parser_fail("struct { extern var foo: bar; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 9});

    helpers::test_parser_fail("struct { defer {}; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 9},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 19uz}});
}

TEST_CASE("Illegal union field name") {
    helpers::test_parser_fail(
        "union { 2: i32 };",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 8});
}

TEST_CASE("Illegal union field type") {
    helpers::test_parser_fail("union { a: 2 };",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_EXPLICIT_TYPE, 0, 9});
}

TEST_CASE("Empty union") {
    helpers::test_parser_fail("union { };", syntax::Diagnostic{syntax::Error::EMPTY_UNION, 0, 0});
}

TEST_CASE("Empty union with decl") {
    helpers::test_parser_fail("union { const b := fn(&self, a: A): C { c; }; };",
                              syntax::Diagnostic{syntax::Error::EMPTY_UNION, 0, 0});
}

TEST_CASE("Out of order union") {
    helpers::test_parser_fail("union { a: i32, const b := fn(&self, a: A): C { c; }; b: i32, };",
                              syntax::Diagnostic{"Expected token SEMICOLON, found COMMA",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 60uz}});
}

TEST_CASE("Non-static non-function union decl") {
    helpers::test_parser_fail("union { a: i32, const b := 2; };",
                              syntax::Diagnostic{syntax::Error::INVALID_MEMBER, 0, 16});
}

} // namespace porpoise::tests

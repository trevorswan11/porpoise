#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/statements/import.hpp"

namespace porpoise::tests {

TEST_CASE("Module imports") {
    helpers::test_stmt(
        "import std;",
        ast::ImportStatement{Token{keywords::IMPORT}, helpers::make_ident("std"), {}});

    helpers::test_stmt("import std as stud;",
                       ast::ImportStatement{Token{keywords::IMPORT},
                                            helpers::make_ident("std"),
                                            helpers::make_ident("stud")});
}

TEST_CASE("User imports") {
    helpers::test_stmt(R"(import "ast/node.p" as node;)",
                       ast::ImportStatement{Token{keywords::IMPORT},
                                            make_box<ast::StringExpression>(
                                                Token{TokenType::STRING, R"("ast/node.p")"},
                                                std::string{"ast/node.p"}),
                                            helpers::make_ident("node")});
}

TEST_CASE("Incorrect module imports ") {
    helpers::test_fail("import 2;", ParserDiagnostic{ParserError::ILLEGAL_IMPORT_TYPE, 1, 8});
    helpers::test_fail("import as 2;", ParserDiagnostic{ParserError::ILLEGAL_IMPORT_TYPE, 1, 8});
    helpers::test_fail("import 2 as 3;", ParserDiagnostic{ParserError::ILLEGAL_IMPORT_TYPE, 1, 8});
    helpers::test_fail(
        "import std as 2;",
        ParserDiagnostic{
            "Expected token IDENT, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 15});
}

TEST_CASE("Incorrect user imports ") {
    helpers::test_fail(R"(import "";)", ParserDiagnostic{ParserError::EMPTY_USER_IMPORT, 1, 8});
    helpers::test_fail(R"(import "" as e;)",
                       ParserDiagnostic{ParserError::EMPTY_USER_IMPORT, 1, 8});
    helpers::test_fail(R"(import "ast/node.p";)",
                       ParserDiagnostic{ParserError::USER_IMPORT_MISSING_ALIAS, 1, 1});
    helpers::test_fail(
        R"(import "ast/node.p" as 2;)",
        ParserDiagnostic{
            "Expected token IDENT, found INT_10", ParserError::UNEXPECTED_TOKEN, 1, 24});
}

TEST_CASE("Non-terminated imports") {
    helpers::test_fail(
        "import std",
        ParserDiagnostic{
            "Expected token SEMICOLON, found END", ParserError::UNEXPECTED_TOKEN, 1, 11});
}

} // namespace porpoise::tests

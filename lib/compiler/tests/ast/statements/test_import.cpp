#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Library imports") {
    helpers::test_stmt("module;", ast::ModuleStatement{syntax::Token{keywords::MODULE}});

    helpers::test_stmt("import std;",
                       ast::ImportStatement{syntax::Token{keywords::IMPORT},
                                            ast::LibraryImport{helpers::make_ident("std"), {}}});

    helpers::test_stmt("import std as stud;",
                       ast::ImportStatement{syntax::Token{keywords::IMPORT},
                                            ast::LibraryImport{helpers::make_ident("std"),
                                                               helpers::make_ident<true>("stud")}});
}

TEST_CASE("File imports") {
    helpers::test_stmt(
        R"(import "ast/node.p" as node;)",
        ast::ImportStatement{
            syntax::Token{keywords::IMPORT},
            ast::FileImport{helpers::make_primitive<ast::StringExpression>(R"("ast/node.p")"),
                            helpers::make_ident("node")}});
}

TEST_CASE("Incorrect module statement") {
    helpers::test_parser_fail(
        "module",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found END", syntax::ParserError::UNEXPECTED_TOKEN, 0, 6});
    helpers::test_parser_fail(
        "module std;",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found IDENT", syntax::ParserError::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Incorrect library imports") {
    helpers::test_parser_fail(
        "import 2;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail(
        "import as 2;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail(
        "import 2 as 3;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail(
        "import std as 2;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Incorrect file imports ") {
    helpers::test_parser_fail(
        R"(import "";)", syntax::ParserDiagnostic{syntax::ParserError::EMPTY_USER_IMPORT, 0, 7});
    helpers::test_parser_fail(
        R"(import "" as e;)",
        syntax::ParserDiagnostic{syntax::ParserError::EMPTY_USER_IMPORT, 0, 7});
    helpers::test_parser_fail(
        R"(import "ast/node.p";)",
        syntax::ParserDiagnostic{syntax::ParserError::USER_IMPORT_MISSING_ALIAS, 0, 0});
    helpers::test_parser_fail(
        R"(import "ast/node.p" as 2;)",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 0, 23});
}

TEST_CASE("Non-terminated imports") {
    helpers::test_parser_fail(
        "import std",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found END", syntax::ParserError::UNEXPECTED_TOKEN, 0, 10});
}

} // namespace porpoise::tests

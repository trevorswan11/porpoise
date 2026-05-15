#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

#include "syntax/keywords.hh"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Non-terminated block") {
    helpers::test_parser_fail(
        "{ ",
        syntax::Diagnostic{
            "Expected token RBRACE, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 2});
}

namespace {

auto test_decl_fail(std::initializer_list<syntax::Keyword> modifiers,
                    syntax::Diagnostic&&                   expected_error,
                    std::string_view                       init = "a := 2;") -> void {
    std::stringstream ss;
    for (const auto& keyword : modifiers) { ss << keyword.first << " "; }
    ss << init;
    helpers::test_parser_fail(ss.view(), std::move(expected_error));
}

} // namespace

TEST_CASE("Mutability restrictions") {
    const std::array contending_mut{keywords::CONSTEXPR, keywords::VAR, keywords::CONSTANT};
    for (const auto& mut : helpers::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
    }
    test_decl_fail({keywords::CONSTEXPR, keywords::VAR, keywords::CONSTANT},
                   syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
}

TEST_CASE("CONSTEXPR restrictions") {
    const std::array contending_mut{keywords::EXTERN, keywords::CONSTEXPR};
    for (const auto& mut : helpers::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
    }
    test_decl_fail({keywords::EXTERN, keywords::CONSTEXPR},
                   syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
}

TEST_CASE("ABI/Linkage restrictions") {
    const std::array contending_mut{keywords::EXTERN, keywords::EXPORT};
    for (const auto& mut : helpers::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
    }
    test_decl_fail({keywords::EXTERN, keywords::EXPORT},
                   syntax::Diagnostic{syntax::Error::ILLEGAL_DECL_MODIFIERS, 0, 0});
}

TEST_CASE("Extern requirements") {
    test_decl_fail({keywords::EXTERN, keywords::CONSTANT},
                   syntax::Diagnostic{syntax::Error::EXTERN_VALUE_INITIALIZED, 0, 0});
    test_decl_fail({keywords::EXTERN, keywords::VAR},
                   syntax::Diagnostic{syntax::Error::EXTERN_VALUE_INITIALIZED, 0, 0});
}

TEST_CASE("Constant requirements") {
    test_decl_fail({keywords::CONSTANT},
                   syntax::Diagnostic{syntax::Error::CONST_DECL_MISSING_VALUE, 0, 0},
                   "a: i32;");
    test_decl_fail({keywords::CONSTEXPR},
                   syntax::Diagnostic{syntax::Error::CONST_DECL_MISSING_VALUE, 0, 0},
                   "a: i32;");
}

TEST_CASE("Non-terminated decls") {
    helpers::test_parser_fail(
        "var a: i32 = 2",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Duplicate declaration modifier") {
    helpers::test_parser_fail("var var a: i32;",
                              syntax::Diagnostic{syntax::Error::DUPLICATE_DECL_MODIFIER, 0, 4});
}

TEST_CASE("Undefined declaration without type") {
    helpers::test_parser_fail("var a := undefined;",
                              syntax::Diagnostic{"Undefined declarations require an explicit type",
                                                 syntax::Error::UNDEFINED_DECL_MISSING_TYPE,
                                                 std::pair{0uz, 0uz}});
}

TEST_CASE("Illegal deferred statements") {
    helpers::test_parser_fail("defer import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_DEFERRED_STATEMENT, 0, 6});
    helpers::test_parser_fail("defer return 3;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_DEFERRED_STATEMENT, 0, 6});
    helpers::test_parser_fail("defer var a: i32;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_DEFERRED_STATEMENT, 0, 6});
    helpers::test_parser_fail("defer using a = i32;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_DEFERRED_STATEMENT, 0, 6});
}

TEST_CASE("Missing deferred statements") {
    helpers::test_parser_fail("defer",
                              syntax::Diagnostic{syntax::Error::DEFER_MISSING_DEFERREE, 0, 0});
    helpers::test_parser_fail("defer;",
                              syntax::Diagnostic{syntax::Error::DEFER_MISSING_DEFERREE, 0, 0});
}

TEST_CASE("Misplaced correct discardedstatement") {
    helpers::test_parser_fail(
        "_ = import std;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 4uz}});
}

TEST_CASE("Misplaced incorrect discarded statement") {
    helpers::test_parser_fail(
        "_ = import 3;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 4uz}});
}

TEST_CASE("Missing discardee") {
    helpers::test_parser_fail("_ = ",
                              syntax::Diagnostic{syntax::Error::DISCARD_MISSING_DISCARDEE, 0, 2});
    helpers::test_parser_fail("_ = ;",
                              syntax::Diagnostic{syntax::Error::DISCARD_MISSING_DISCARDEE, 0, 2});
}

TEST_CASE("Incorrect library imports") {
    helpers::test_parser_fail("import 2;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail("import as 2;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail("import 2 as 3;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IMPORT_TYPE, 0, 7});
    helpers::test_parser_fail(
        "import std as 2;",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Incorrect file imports ") {
    helpers::test_parser_fail(R"(import "";)",
                              syntax::Diagnostic{syntax::Error::EMPTY_USER_IMPORT, 0, 7});
    helpers::test_parser_fail(R"(import "" as e;)",
                              syntax::Diagnostic{syntax::Error::EMPTY_USER_IMPORT, 0, 7});
    helpers::test_parser_fail(R"(import "ast/node.p";)",
                              syntax::Diagnostic{syntax::Error::USER_IMPORT_MISSING_ALIAS, 0, 0});
    helpers::test_parser_fail(
        R"(import "ast/node.p" as 2;)",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 23});
}

TEST_CASE("Non-terminated imports") {
    helpers::test_parser_fail(
        "import std",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 10});
}

TEST_CASE("Incorrectly terminated jumps") {
    const auto inputs = std::to_array<std::string_view>({"return", "continue", "break"});
    for (const auto& input : inputs) {
        helpers::test_parser_fail(input,
                                  syntax::Diagnostic{"Expected token SEMICOLON, found END",
                                                     syntax::Error::UNEXPECTED_TOKEN,
                                                     std::pair{0uz, input.size()}});
    }

    helpers::test_parser_fail(
        "return return",
        syntax::Diagnostic{"No prefix parse function for RETURN(return) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 7uz}});
}

TEST_CASE("Illegal continue/break control flow") {
    helpers::test_parser_fail("continue 4;",
                              syntax::Diagnostic{syntax::Error::VALUED_CONTINUE, 0, 0});

    helpers::test_parser_fail("break 4;",
                              syntax::Diagnostic{syntax::Error::VALUED_BREAK_MISSING_LABEL, 0, 0});
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

TEST_CASE("Missing alias") {
    helpers::test_parser_fail(
        "using &[0x2uz][N]*E;",
        syntax::Diagnostic{
            "Expected token IDENT, found BW_AND", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Missing type") {
    helpers::test_parser_fail(
        "using T;",
        syntax::Diagnostic{
            "Expected token ASSIGN, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Illegal identifier alias") {
    helpers::test_parser_fail(
        "using type = T;",
        syntax::Diagnostic{
            "Expected token IDENT, found TYPE_TYPE", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

} // namespace porpoise::tests

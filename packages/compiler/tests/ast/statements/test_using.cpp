#include <catch2/catch_test_macros.hpp>

#include "ast/helpers.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/statements/using.hpp"

namespace conch::tests {

namespace mods = helpers::type_modifiers;

TEST_CASE("Well formed using statement") {
    helpers::test_stmt(
        "using T = &[0x2uz][N]*E;",
        ast::UsingStatement{
            Token{keywords::USING},
            helpers::make_ident("T"),
            ast::ExplicitType{
                mods::REF,
                ast::ExplicitArrayType{
                    make_box<ast::USizeIntegerExpression>(Token{TokenType::UZINT_16, "0x2uz"},
                                                          0x2uz),
                    make_box<ast::ExplicitType>(
                        mods::BASE,
                        ast::ExplicitArrayType{
                            helpers::make_ident("N"),
                            make_box<ast::ExplicitType>(mods::PTR, helpers::make_ident("E"))})}}});
}

TEST_CASE("Missing alias") {
    helpers::test_fail(
        "using &[0x2uz][N]*E;",
        ParserDiagnostic{
            "Expected token IDENT, found BW_AND", ParserError::UNEXPECTED_TOKEN, 1, 7});
}

TEST_CASE("Missing type") {
    helpers::test_fail(
        "using T;",
        ParserDiagnostic{
            "Expected token ASSIGN, found SEMICOLON", ParserError::UNEXPECTED_TOKEN, 1, 8});
}

TEST_CASE("Illegal identifier alias") {
    helpers::test_fail(
        "using type = T;",
        ParserDiagnostic{
            "Expected token IDENT, found TYPE_TYPE", ParserError::UNEXPECTED_TOKEN, 1, 7});
}

} // namespace conch::tests

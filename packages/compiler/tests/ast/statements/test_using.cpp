#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/statements/using.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

TEST_CASE("Well formed using statement") {
    helpers::test_stmt(
        "using T = &[0x2uz][N]*E;",
        ast::UsingStatement{
            syntax::Token{keywords::USING},
            helpers::make_ident("T"),
            ast::ExplicitType{
                mods::REF,
                ast::ExplicitArrayType{
                    make_box<ast::USizeIntegerExpression>(
                        syntax::Token{syntax::TokenType::UZINT_16, "0x2uz"}, 0x2uz),
                    false,
                    make_box<ast::ExplicitType>(
                        mods::BASE,
                        ast::ExplicitArrayType{
                            helpers::make_ident("N"),
                            false,
                            make_box<ast::ExplicitType>(mods::PTR, helpers::make_ident("E"))})}}});
}

TEST_CASE("Missing alias") {
    helpers::test_parser_fail(
        "using &[0x2uz][N]*E;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found BW_AND", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7});
}

TEST_CASE("Missing type") {
    helpers::test_parser_fail(
        "using T;",
        syntax::ParserDiagnostic{
            "Expected token ASSIGN, found SEMICOLON", syntax::ParserError::UNEXPECTED_TOKEN, 1, 8});
}

TEST_CASE("Illegal identifier alias") {
    helpers::test_parser_fail(
        "using type = T;",
        syntax::ParserDiagnostic{
            "Expected token IDENT, found TYPE_TYPE", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7});
}

} // namespace porpoise::tests

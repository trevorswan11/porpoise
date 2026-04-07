#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Call with no arguments") {
    const syntax::Token func{syntax::TokenType::IDENT, "func"};
    helpers::test_expr_stmt("func();", ast::CallExpression{func, helpers::make_ident(func), {}});
}

TEST_CASE("Trailing comma") {
    const syntax::Token func{keywords::builtins::SIN};
    helpers::test_expr_stmt(
        "@sin(23.6, );",
        ast::CallExpression{
            func,
            helpers::make_ident(func),
            helpers::make_vector<ast::CallArgument>(mem::make_box<ast::F64Expression>(
                syntax::Token{syntax::TokenType::F64, "23.6"}, 23.6))});
}

TEST_CASE("Builtin with multiple arguments") {
    const syntax::Token func{keywords::builtins::PTR_ADD};
    helpers::test_expr_stmt(
        "@ptrAdd(a, 4uz);",
        ast::CallExpression{func,
                            helpers::make_ident(func),
                            helpers::make_vector<ast::CallArgument>(
                                helpers::make_ident("a"),
                                mem::make_box<ast::USizeIntegerExpression>(
                                    syntax::Token{syntax::TokenType::UZINT_10, "4uz"}, 4uz))});
}

TEST_CASE("Type arguments in call") {
    const syntax::Token func{syntax::TokenType::IDENT, "a"};
    helpers::test_expr_stmt(
        "a(&mut r, t, *[N]i32, [:0]u8);",
        ast::CallExpression{
            func,
            helpers::make_ident(func),
            helpers::make_vector<ast::CallArgument>(
                mem::make_box<ast::ReferenceExpression>(syntax::Token{operators::AND_MUT},
                                                        helpers::make_ident("r")),
                helpers::make_ident("t"),
                ast::ExplicitType{
                    mods::PTR,
                    ast::ExplicitArrayType{
                        helpers::make_ident("N"),
                        false,
                        mem::make_box<ast::ExplicitType>(mods::BASE, helpers::make_ident("i32"))}},
                ast::ExplicitType{
                    mods::BASE,
                    ast::ExplicitArrayType{{},
                                           true,
                                           mem::make_box<ast::ExplicitType>(
                                               mods::BASE, helpers::make_ident("u8"))}})});
}

TEST_CASE("No arguments with comma") {
    helpers::test_parser_fail(
        "func(,)",
        syntax::ParserDiagnostic{syntax::ParserError::COMMA_WITH_MISSING_CALL_ARGUMENT, 1, 6});
}

TEST_CASE("Non-comma separated arguments") {
    helpers::test_parser_fail(
        "func(1 2)",
        syntax::ParserDiagnostic{
            "Expected token COMMA, found INT_10", syntax::ParserError::UNEXPECTED_TOKEN, 1, 8});
}

} // namespace porpoise::tests

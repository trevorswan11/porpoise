#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

using Parameters = std::vector<ast::FunctionParameter>;

namespace helpers {

auto function_expr_from(Optional<ast::SelfParameter>&&     self,
                        Parameters&&                       parameters,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = std::nullopt)
    -> ast::FunctionExpression {
    return ast::FunctionExpression{syntax::Token{keywords::FN},
                                   std::move(self),
                                   std::move(parameters),
                                   std::move(return_type),
                                   std::move(block)};
}

auto function_expr_from(Optional<ast::SelfParameter>&&     self,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = std::nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(std::move(self), {}, std::move(return_type), std::move(block));
}

auto function_expr_from(Parameters&&                       parameters,
                        ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = std::nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(
        std::nullopt, std::move(parameters), std::move(return_type), std::move(block));
}

auto function_expr_from(ast::ExplicitType&&                return_type,
                        Optional<Box<ast::BlockStatement>> block = std::nullopt)
    -> ast::FunctionExpression {
    return function_expr_from(std::nullopt, {}, std::move(return_type), std::move(block));
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

TEST_CASE("Function without body, self, or parameters") {
    helpers::test_expr_stmt(
        "fn(): int;",
        helpers::function_expr_from(ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with self but no parameters or body") {
    helpers::test_expr_stmt(
        "fn(&self): int;",
        helpers::function_expr_from(ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with parameters but no self or body") {
    helpers::test_expr_stmt(
        "fn(a: A, b: *B): int;",
        helpers::function_expr_from(
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with self & parameters but no body") {
    helpers::test_expr_stmt(
        "fn(&mut this, a: A, b: *B): int;",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_REF, helpers::make_ident("this")},
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")}));
}

TEST_CASE("Function with type types") {
    helpers::test_expr_stmt("fn(self, A: type): type;",
                            helpers::function_expr_from(
                                ast::SelfParameter{mods::BASE, helpers::make_ident("self")},
                                helpers::make_parameters(ast::FunctionParameter{
                                    helpers::make_ident("A"),
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("type")}}),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("type")}));
}

TEST_CASE("Full function expression") {
    helpers::test_expr_stmt(
        "fn(*mut this, a: A, b: *B, ): int { c; };",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_PTR, helpers::make_ident("this")},
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("int")},
            helpers::make_expr_block_stmt(helpers::ident_from("c"))));
}

TEST_CASE("Function missing return type") {
    helpers::test_parser_fail(
        "fn(*mut this, a: A, b: *B, );",
        syntax::ParserDiagnostic{
            "Expected token COLON, found SEMICOLON", syntax::ParserError::UNEXPECTED_TOKEN, 1, 29});

    helpers::test_parser_fail(
        "fn(*mut this, a: A, b: *B, ): ;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 1,
                                 31});
}

TEST_CASE("Function parameter missing type") {
    helpers::test_parser_fail(
        "fn(*mut this, a): int;",
        syntax::ParserDiagnostic{
            "Expected token COLON, found RPAREN", syntax::ParserError::UNEXPECTED_TOKEN, 1, 16});
}

TEST_CASE("Out-of-place self parameter") {
    helpers::test_parser_fail(
        "fn(a: A, &self): int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 10});

    helpers::test_parser_fail(
        "fn(a: A, self): int;",
        syntax::ParserDiagnostic{
            "Expected token COLON, found RPAREN", syntax::ParserError::UNEXPECTED_TOKEN, 1, 14});
}

TEST_CASE("Default function parameter") {
    helpers::test_parser_fail(
        "fn(a: A = 2): int;",
        syntax::ParserDiagnostic{syntax::ParserError::FUNCTION_PARAMETER_HAS_DEFAULT_VALUE, 1, 5});
}

TEST_CASE("Noreturn function types") {
    helpers::test_parser_fail(
        "fn(a: &noreturn): int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 1, 7});
    helpers::test_parser_fail(
        "fn(a: noreturn): int;",
        syntax::ParserDiagnostic{syntax::ParserError::FUNCTION_PARAMETER_IS_NORETURN, 1, 5});
    helpers::test_parser_fail(
        "fn(a: A): &noreturn;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 1, 11});
}

TEST_CASE("Illegal type function types") {
    helpers::test_parser_fail(
        "fn(A: &type): int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 1, 7});
    helpers::test_parser_fail(
        "fn(A: type): &type;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 1, 14});
}

TEST_CASE("Non-terminated parameter list") {
    helpers::test_parser_fail(
        "fn(a: A, : int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 10});
}

} // namespace porpoise::tests

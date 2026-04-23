#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

using Parameters = std::vector<ast::FunctionParameter>;

namespace helpers {

auto function_expr_from(opt::Option<ast::SelfParameter>&&     self,
                        Parameters&&                          parameters,
                        ast::ExplicitType&&                   return_type,
                        mem::NullableBox<ast::BlockStatement> block = nullptr,
                        bool variadic = false) -> ast::FunctionExpression {
    return ast::FunctionExpression{syntax::Token{keywords::FN},
                                   std::move(self),
                                   std::move(parameters),
                                   variadic,
                                   std::move(return_type),
                                   std::move(block)};
}

auto function_expr_from(opt::Option<ast::SelfParameter>&&     self,
                        ast::ExplicitType&&                   return_type,
                        mem::NullableBox<ast::BlockStatement> block = nullptr)
    -> ast::FunctionExpression {
    return function_expr_from(std::move(self), {}, std::move(return_type), std::move(block));
}

auto function_expr_from(Parameters&&                          parameters,
                        ast::ExplicitType&&                   return_type,
                        mem::NullableBox<ast::BlockStatement> block = nullptr)
    -> ast::FunctionExpression {
    return function_expr_from(
        opt::none, std::move(parameters), std::move(return_type), std::move(block));
}

auto function_expr_from(ast::ExplicitType&&                   return_type,
                        mem::NullableBox<ast::BlockStatement> block = nullptr)
    -> ast::FunctionExpression {
    return function_expr_from(opt::none, {}, std::move(return_type), std::move(block));
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

TEST_CASE("Function without self or parameters") {
    helpers::test_expr_stmt(
        "fn(): i32 {};",
        helpers::function_expr_from(ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                                    helpers::make_block_stmt<true>()));
}

TEST_CASE("Function with self but no parameters") {
    helpers::test_expr_stmt(
        "fn(&self): i32 {};",
        helpers::function_expr_from(ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                                    helpers::make_block_stmt<true>()));
}

TEST_CASE("Function with parameters but no self") {
    helpers::test_expr_stmt(
        "fn(a: A, b: *B): i32 {};",
        helpers::function_expr_from(
            helpers::make_parameters(ast::FunctionParameter{helpers::make_ident("a"),
                                                            {mods::BASE, helpers::make_ident("A")}},
                                     ast::FunctionParameter{helpers::make_ident("b"),
                                                            {mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
            helpers::make_block_stmt<true>()));
}

TEST_CASE("Function with self & parameters") {
    helpers::test_expr_stmt(
        "fn(&mut this, a: A, b: *B): i32 {};",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_REF, helpers::make_ident("this")},
            helpers::make_parameters(ast::FunctionParameter{helpers::make_ident("a"),
                                                            {mods::BASE, helpers::make_ident("A")}},
                                     ast::FunctionParameter{helpers::make_ident("b"),
                                                            {mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
            helpers::make_block_stmt<true>()));
}

TEST_CASE("Functions with variadic parameter") {
    SECTION("Sole variadic") {
        helpers::test_expr_stmt(
            "fn(...): i32 {};",
            helpers::function_expr_from(opt::none,
                                        Parameters{},
                                        ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                                        helpers::make_block_stmt<true>(),
                                        true));
    }

    SECTION("Variadic following self parameter") {
        helpers::test_expr_stmt("fn(&mut this, ...): i32 {};",
                                helpers::function_expr_from(
                                    ast::SelfParameter{mods::MUT_REF, helpers::make_ident("this")},
                                    Parameters{},
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                                    helpers::make_block_stmt<true>(),
                                    true));
    }

    SECTION("Variadic following standard parameter") {
        helpers::test_expr_stmt(
            "fn(a: A, ...): i32 {};",
            helpers::function_expr_from(
                opt::none,
                helpers::make_parameters(ast::FunctionParameter{
                    helpers::make_ident("a"), {mods::BASE, helpers::make_ident("A")}}),
                ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
                helpers::make_block_stmt<true>(),
                true));
    }
}

TEST_CASE("Full function signature") {
    helpers::test_expr_stmt(
        "fn(&self, a: A, ...): *i32 {};",
        helpers::function_expr_from(
            ast::SelfParameter{mods::REF, helpers::make_ident("self")},
            helpers::make_parameters(ast::FunctionParameter{
                helpers::make_ident("a"), {mods::BASE, helpers::make_ident("A")}}),
            ast::ExplicitType{mods::PTR, helpers::make_ident("i32")},
            helpers::make_block_stmt<true>(),
            true));
}

TEST_CASE("Function with type types") {
    helpers::test_expr_stmt("fn(self, A: type): type {};",
                            helpers::function_expr_from(
                                ast::SelfParameter{mods::BASE, helpers::make_ident("self")},
                                helpers::make_parameters(ast::FunctionParameter{
                                    helpers::make_ident("A"),
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("type")}}),
                                ast::ExplicitType{mods::BASE, helpers::make_ident("type")},
                                helpers::make_block_stmt<true>()));
}

TEST_CASE("Full function expression") {
    helpers::test_expr_stmt(
        "fn(*mut this, a: A, b: *B, ): i32 { c; };",
        helpers::function_expr_from(
            ast::SelfParameter{mods::MUT_PTR, helpers::make_ident("this")},
            helpers::make_parameters(
                ast::FunctionParameter{helpers::make_ident("a"),
                                       ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                ast::FunctionParameter{helpers::make_ident("b"),
                                       ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
            ast::ExplicitType{mods::BASE, helpers::make_ident("i32")},
            helpers::make_expr_block_stmt<true>(helpers::ident_from("c"))));
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
                                 std::pair{1uz, 31uz}});

    helpers::test_parser_fail(
        "fn(*mut this, a: A, b: *B, ): ",
        syntax::ParserDiagnostic{syntax::ParserError::MISSING_EXPLICIT_TYPE, 1, 29});
}

TEST_CASE("Function parameter missing type") {
    helpers::test_parser_fail(
        "fn(*mut this, a): i32;",
        syntax::ParserDiagnostic{
            "Expected token COLON, found RPAREN", syntax::ParserError::UNEXPECTED_TOKEN, 1, 16});
}

TEST_CASE("Out-of-place self parameter") {
    helpers::test_parser_fail(
        "fn(a: A, &self): i32;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 10});

    helpers::test_parser_fail(
        "fn(a: A, self): i32;",
        syntax::ParserDiagnostic{
            "Expected token COLON, found RPAREN", syntax::ParserError::UNEXPECTED_TOKEN, 1, 14});
}

TEST_CASE("Out-of-place variadic parameter") {
    helpers::test_parser_fail(
        "fn(a: A, ..., b: B): i32;",
        syntax::ParserDiagnostic{
            "Expected token RPAREN, found IDENT", syntax::ParserError::UNEXPECTED_TOKEN, 1, 15});
}

TEST_CASE("Default function parameter") {
    helpers::test_parser_fail(
        "fn(a: A = 2): i32;",
        syntax::ParserDiagnostic{syntax::ParserError::FUNCTION_PARAMETER_HAS_DEFAULT_VALUE, 1, 5});
}

TEST_CASE("Noreturn function types") {
    helpers::test_parser_fail(
        "fn(a: &noreturn): i32;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 1, 7});
    helpers::test_parser_fail(
        "fn(a: noreturn): i32;",
        syntax::ParserDiagnostic{syntax::ParserError::FUNCTION_PARAMETER_IS_NORETURN, 1, 5});
    helpers::test_parser_fail(
        "fn(a: A): &noreturn;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 1, 11});
}

TEST_CASE("Illegal type function types") {
    helpers::test_parser_fail(
        "fn(A: &type): i32;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 1, 7});
    helpers::test_parser_fail(
        "fn(A: type): &type;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 1, 14});
}

TEST_CASE("Non-terminated parameter list") {
    helpers::test_parser_fail(
        "fn(a: A, : i32;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 10});
}

} // namespace porpoise::tests

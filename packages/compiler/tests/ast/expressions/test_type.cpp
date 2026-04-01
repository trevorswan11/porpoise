#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"
#include "ast/expressions/type_modifiers.hpp"
#include "ast/statements/block.hpp" // IWYU pragma: keep
#include "ast/statements/declaration.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

namespace helpers {

auto test_type_expr(std::string_view type_str, ast::ExplicitType&& expected) -> void {
    const auto input = fmt::format("var a: {};", type_str);
    helpers::test_stmt(
        input,
        ast::DeclStatement{syntax::Token{keywords::VAR},
                           helpers::make_ident("a"),
                           mem::make_box<ast::TypeExpression>(
                               syntax::Token{syntax::TokenType::COLON, ":"}, std::move(expected)),
                           std::nullopt,
                           ast::DeclModifiers::VARIABLE});
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

using Parameters = std::vector<ast::FunctionParameter>;

TEST_CASE("Indent type") {
    helpers::test_type_expr("int", ast::ExplicitType{mods::BASE, helpers::make_ident("int")});
    helpers::test_type_expr("*int", ast::ExplicitType{mods::PTR, helpers::make_ident("int")});
}

TEST_CASE("Function types") {
    helpers::test_type_expr(
        "fn(): noreturn",
        ast::ExplicitType{mods::BASE,
                          mem::make_box<ast::FunctionExpression>(
                              syntax::Token{keywords::FN},
                              std::nullopt,
                              Parameters{},
                              ast::ExplicitType{mods::BASE, helpers::make_ident("noreturn")},
                              std::nullopt)});

    helpers::test_type_expr(
        "fn(&self): *mut E",
        ast::ExplicitType{mods::BASE,
                          mem::make_box<ast::FunctionExpression>(
                              syntax::Token{keywords::FN},
                              std::nullopt,
                              helpers::make_parameters(
                                  ast::FunctionParameter{{mods::REF, helpers::make_ident("self")}}),
                              ast::ExplicitType{mods::MUT_PTR, helpers::make_ident("E")},
                              std::nullopt)});
}

TEST_CASE("Array type") {
    helpers::test_type_expr(
        "[5uz]*ulong",
        ast::ExplicitType{
            mods::BASE,
            ast::ExplicitArrayType{
                mem::make_box<ast::USizeIntegerExpression>(
                    syntax::Token{syntax::TokenType::UZINT_10, "5uz"}, 5uz),
                false,
                mem::make_box<ast::ExplicitType>(mods::PTR, helpers::make_ident("ulong"))}});
}

TEST_CASE("Slice type") {
    helpers::test_type_expr(
        "[]*ulong",
        ast::ExplicitType{mods::BASE,
                          ast::ExplicitArrayType{{},
                                                 false,
                                                 mem::make_box<ast::ExplicitType>(
                                                     mods::PTR, helpers::make_ident("ulong"))}});
}

TEST_CASE("Recursive types") {
    helpers::test_type_expr(
        "&[S]&*mut T",
        ast::ExplicitType{
            mods::REF,
            ast::ExplicitArrayType{
                helpers::make_ident("S"),
                false,
                mem::make_box<ast::ExplicitType>(
                    mods::REF,
                    mem::make_box<ast::ExplicitType>(mods::MUT_PTR, helpers::make_ident("T")))}});
}

TEST_CASE("Complex function type (holistic)") {
    helpers::test_type_expr(
        "*fn(&a, *mut B): &[0x2uz][N:0]*E",
        ast::ExplicitType{
            mods::PTR,
            mem::make_box<ast::FunctionExpression>(
                syntax::Token{keywords::FN},
                std::nullopt,
                helpers::make_parameters(
                    ast::FunctionParameter{{mods::REF, helpers::make_ident("a")}},
                    ast::FunctionParameter{{mods::MUT_PTR, helpers::make_ident("B")}}),
                ast::ExplicitType{
                    mods::REF,
                    ast::ExplicitArrayType{
                        mem::make_box<ast::USizeIntegerExpression>(
                            syntax::Token{syntax::TokenType::UZINT_16, "0x2uz"}, 0x2uz),
                        false,
                        mem::make_box<ast::ExplicitType>(
                            mods::BASE,
                            ast::ExplicitArrayType{helpers::make_ident("N"),
                                                   true,
                                                   mem::make_box<ast::ExplicitType>(
                                                       mods::PTR, helpers::make_ident("E"))})}},
                std::nullopt)});
}

TEST_CASE("Volatile restricted to declarations") {
    helpers::test_parser_fail(
        "var a: volatile int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_EXPLICIT_TYPE, 1, 6});
}

TEST_CASE("Array type requirement") {
    helpers::test_parser_fail(
        "var a: [9]int;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 1, 9});
    helpers::test_parser_fail(
        R"(var a: ["e"]int;)",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 1, 9});
}

TEST_CASE("Function type restrictions") {
    const auto illegals = std::to_array<std::string_view>({
        "var a: *mut fn(): void;",
        "var a: &fn(): void;",
        "var a: &mut fn(): void;",
    });
    for (const auto& illegal : illegals) {
        helpers::test_parser_fail(
            illegal,
            syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_FUNCTION_TYPE_MODIFIER, 1, 8});
    }
}

TEST_CASE("Bodied function type") {
    helpers::test_parser_fail(
        "var a: *mut fn(): void { b; };",
        syntax::ParserDiagnostic{syntax::ParserError::EXPLICIT_FN_TYPE_HAS_BODY, 1, 13},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 29uz}});
}

TEST_CASE("Function return type restrictions") {
    helpers::test_parser_fail(
        "var a: fn(): &void;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_VOID_TYPE_MODIFIER, 1, 14});
    helpers::test_parser_fail(
        "var a: fn(): &type;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 1, 14});
    helpers::test_parser_fail(
        "var a: fn(): &noreturn;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 1, 14});
}

} // namespace porpoise::tests

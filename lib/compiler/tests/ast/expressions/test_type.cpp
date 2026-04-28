#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

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
                           {},
                           ast::DeclModifiers::VARIABLE});
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

using Parameters = std::vector<ast::FunctionParameter>;
using Arguments  = std::vector<ast::CallArgument>;

TEST_CASE("Named types") {
    SECTION("Shallow types") {
        helpers::test_type_expr("i32", ast::ExplicitType{mods::BASE, helpers::make_ident("i32")});
        helpers::test_type_expr("*i32", ast::ExplicitType{mods::PTR, helpers::make_ident("i32")});

        const syntax::Token a{syntax::TokenType::IDENT, "a"};
        helpers::test_type_expr("a()",
                                ast::ExplicitType{mods::BASE,
                                                  mem::make_box<ast::CallExpression>(
                                                      a, helpers::make_ident(a), Arguments{})});
    }

    SECTION("Scoped types") {
        const syntax::Token outer{syntax::TokenType::IDENT, "std"};
        helpers::test_type_expr(
            "std::Io",
            ast::ExplicitType{mods::BASE,
                              mem::make_box<ast::ScopeResolutionExpression>(
                                  outer, helpers::make_ident(outer), helpers::make_ident("Io"))});
        helpers::test_type_expr(
            "*std::Io",
            ast::ExplicitType{mods::PTR,
                              mem::make_box<ast::ScopeResolutionExpression>(
                                  outer, helpers::make_ident(outer), helpers::make_ident("Io"))});
        helpers::test_type_expr(
            "std::ArrayList(u8)",
            ast::ExplicitType{
                mods::BASE,
                mem::make_box<ast::CallExpression>(
                    outer,
                    mem::make_box<ast::ScopeResolutionExpression>(
                        outer, helpers::make_ident(outer), helpers::make_ident("ArrayList")),
                    helpers::make_vector<ast::CallArgument>(helpers::make_ident("u8")))});
    }
}

TEST_CASE("Function types") {
    helpers::test_type_expr(
        "fn(): noreturn",
        ast::ExplicitType{mods::BASE,
                          mem::make_box<ast::FunctionExpression>(
                              syntax::Token{keywords::FN},
                              opt::none,
                              Parameters{},
                              false,
                              ast::ExplicitType{mods::BASE, helpers::make_ident("noreturn")},
                              nullptr)});

    helpers::test_type_expr(
        "fn(&self): *mut E",
        ast::ExplicitType{mods::BASE,
                          mem::make_box<ast::FunctionExpression>(
                              syntax::Token{keywords::FN},
                              opt::none,
                              helpers::make_parameters(
                                  ast::FunctionParameter{{mods::REF, helpers::make_ident("self")}}),
                              false,
                              ast::ExplicitType{mods::MUT_PTR, helpers::make_ident("E")},
                              nullptr)});
}

TEST_CASE("Array type") {
    helpers::test_type_expr(
        "[5uz]*u64",
        ast::ExplicitType{
            mods::BASE,
            ast::ExplicitArrayType{
                helpers::make_primitive<ast::USizeExpression, true>("5uz"),
                false,
                mem::make_box<ast::ExplicitType>(mods::PTR, helpers::make_ident("u64"))}});
}

TEST_CASE("Slice type") {
    helpers::test_type_expr(
        "[]*u64",
        ast::ExplicitType{mods::BASE,
                          ast::ExplicitArrayType{{},
                                                 false,
                                                 mem::make_box<ast::ExplicitType>(
                                                     mods::PTR, helpers::make_ident("u64"))}});
}

TEST_CASE("Recursive types") {
    helpers::test_type_expr(
        "&[S]&*mut T",
        ast::ExplicitType{
            mods::REF,
            ast::ExplicitArrayType{
                helpers::make_ident<true>("S"),
                false,
                mem::make_box<ast::ExplicitType>(
                    mods::REF,
                    mem::make_box<ast::ExplicitType>(mods::MUT_PTR, helpers::make_ident("T")))}});
}

TEST_CASE("Complex function type (holistic)") {
    helpers::test_type_expr(
        "*fn(&a, *mut B, ...): &[0x2uz][N:0]*E",
        ast::ExplicitType{
            mods::PTR,
            mem::make_box<ast::FunctionExpression>(
                syntax::Token{keywords::FN},
                opt::none,
                helpers::make_parameters(
                    ast::FunctionParameter{{mods::REF, helpers::make_ident("a")}},
                    ast::FunctionParameter{{mods::MUT_PTR, helpers::make_ident("B")}}),
                true,
                ast::ExplicitType{
                    mods::REF,
                    ast::ExplicitArrayType{
                        helpers::make_primitive<ast::USizeExpression, true>("0x2uz"),
                        false,
                        mem::make_box<ast::ExplicitType>(
                            mods::BASE,
                            ast::ExplicitArrayType{helpers::make_ident<true>("N"),
                                                   true,
                                                   mem::make_box<ast::ExplicitType>(
                                                       mods::PTR, helpers::make_ident("E"))})}},
                nullptr)});
}

TEST_CASE("Union inline types") {
    helpers::test_type_expr(
        "&union { a: i32, b: &mut T, };",
        ast::ExplicitType{
            mods::REF,
            mem::make_box<ast::UnionExpression>(
                syntax::Token{keywords::UNION},
                helpers::make_vector<ast::UnionField>(
                    ast::UnionField{helpers::make_ident("a"),
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}},
                    ast::UnionField{helpers::make_ident("b"),
                                    ast::ExplicitType{mods::MUT_REF, helpers::make_ident("T")}}),
                helpers::make_decls())});
}

TEST_CASE("Struct inline types") {
    helpers::test_type_expr(
        "*struct { var b: Foo = bar; };",
        ast::ExplicitType{
            mods::PTR,
            mem::make_box<ast::StructExpression>(
                syntax::Token{keywords::STRUCT},
                helpers::make_decls(ast::DeclStatement{
                    syntax::Token{keywords::VAR},
                    helpers::make_ident("b"),
                    mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                                       ast::ExplicitType{
                                                           mods::BASE,
                                                           helpers::make_ident("Foo"),
                                                       }),
                    helpers::make_ident<true>("bar"),
                    ast::DeclModifiers::VARIABLE,
                }))});
}

TEST_CASE("Enum inline types") {
    helpers::test_type_expr(
        "enum : u64 {RED = 3u, B, };",
        ast::ExplicitType{
            mods::BASE,
            mem::make_box<ast::EnumExpression>(
                syntax::Token{keywords::ENUM},
                helpers::make_ident<true>("u64"),
                helpers::make_vector<ast::Enumeration>(
                    ast::Enumeration{helpers::make_ident("RED"),
                                     helpers::make_primitive<ast::U32Expression, true>("3u")},
                    ast::Enumeration{helpers::make_ident("B"), {}}),
                helpers::make_decls())});
}

TEST_CASE("Volatile restricted to declarations") {
    helpers::test_parser_fail(
        "var a: volatile i32;",
        syntax::ParserDiagnostic{"No prefix parse function for VOLATILE(volatile) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{0uz, 7uz}});
}

TEST_CASE("Array type requirement") {
    helpers::test_parser_fail(
        "var a: [9]i32;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 0, 8});
    helpers::test_parser_fail(
        R"(var a: ["e"]i32;)",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_ARRAY_SIZE_TYPE, 0, 8});
}

TEST_CASE("Function type restrictions") {
    const auto illegals = std::to_array<std::string_view>({
        "var a: &fn(): void;",
        "var a: &mut fn(): void;",
    });
    for (const auto& illegal : illegals) {
        helpers::test_parser_fail(
            illegal,
            syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_FUNCTION_TYPE_MODIFIER, 0, 7});
    }
}

TEST_CASE("Bodied function type") {
    helpers::test_parser_fail(
        "var a: *mut fn(): void { b; };",
        syntax::ParserDiagnostic{syntax::ParserError::EXPLICIT_FN_TYPE_HAS_BODY, 0, 12},
        syntax::ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{0uz, 28uz}});
}

TEST_CASE("Function return type restrictions") {
    helpers::test_parser_fail(
        "var a: fn(): &void;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_VOID_TYPE_MODIFIER, 0, 13});
    helpers::test_parser_fail(
        "var a: fn(): &type;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_TYPE_TYPE_MODIFIER, 0, 13});
    helpers::test_parser_fail(
        "var a: fn(): &noreturn;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_NORETURN_TYPE_MODIFIER, 0, 13});
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Basic enums") {
    helpers::test_expr_stmt("enum {A, B, C};",
                            ast::EnumExpression{syntax::Token{keywords::ENUM},
                                                {},
                                                helpers::make_vector<ast::Enumeration>(
                                                    ast::Enumeration{helpers::make_ident("A"), {}},
                                                    ast::Enumeration{helpers::make_ident("B"), {}},
                                                    ast::Enumeration{helpers::make_ident("C"), {}}),
                                                helpers::make_decls()});

    helpers::test_expr_stmt(
        "enum {A = 1, B = T, };",
        ast::EnumExpression{
            syntax::Token{keywords::ENUM},
            {},
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{helpers::make_ident("A"),
                                 helpers::make_primitive<ast::I32Expression, true>("1")},
                ast::Enumeration{helpers::make_ident("B"), helpers::make_ident<true>("T")}),
            helpers::make_decls()});
}

TEST_CASE("Underlying type") {
    helpers::test_expr_stmt(
        "enum : u64 {RED = 3u, B, };",
        ast::EnumExpression{
            syntax::Token{keywords::ENUM},
            helpers::make_ident<true>("u64"),
            helpers::make_vector<ast::Enumeration>(
                ast::Enumeration{helpers::make_ident("RED"),
                                 helpers::make_primitive<ast::U32Expression, true>("3u")},
                ast::Enumeration{helpers::make_ident("B"), {}}),
            helpers::make_decls()});

    helpers::test_expr_stmt(
        "enum : U {A = 0x4uz};",
        ast::EnumExpression{syntax::Token{keywords::ENUM},
                            helpers::make_ident<true>("U"),
                            helpers::make_vector<ast::Enumeration>(ast::Enumeration{
                                helpers::make_ident("A"),
                                helpers::make_primitive<ast::USizeExpression, true>("0x4uz")}),
                            helpers::make_decls()});
}

TEST_CASE("Enum with decls") {
    const auto test = [](std::string_view input) {
        helpers::test_expr_stmt(
            input,
            ast::EnumExpression{
                syntax::Token{keywords::ENUM},
                helpers::make_ident<true>("i64"),
                helpers::make_vector<ast::Enumeration>(
                    ast::Enumeration{helpers::make_ident("A"),
                                     helpers::make_primitive<ast::I64Expression, true>("2l")}),
                helpers::make_decls(
                    ast::DeclStatement{
                        syntax::Token{keywords::CONST},
                        helpers::make_ident("b"),
                        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS},
                                                           std::nullopt),
                        mem::make_nullable_box<ast::FunctionExpression>(
                            syntax::Token{keywords::FN},
                            ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                            helpers::make_parameters(ast::FunctionParameter{
                                helpers::make_ident("a"), {mods::BASE, helpers::make_ident("A")}}),
                            false,
                            ast::ExplicitType{mods::BASE, helpers::make_ident("C")},
                            helpers::make_expr_block_stmt<true>(helpers::ident_from("c"))),
                        ast::DeclModifiers::CONSTANT,
                    },
                    ast::DeclStatement{
                        syntax::Token{keywords::STATIC},
                        helpers::make_ident("a"),
                        mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS},
                                                           std::nullopt),
                        helpers::make_primitive<ast::I32Expression, true>("2"),
                        ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
                    })});
    };

    constexpr std::string_view format_str =
        "enum : i64 {{ A = 2l{} const b := fn(&self, a: A): C {{ c; }}; static const a := 2; }};";
    test(fmt::format(format_str, ""));
    test(fmt::format(format_str, ","));
}

TEST_CASE("Empty enum") {
    helpers::test_parser_fail("enum {};",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_ENUM, 1, 1});
    helpers::test_parser_fail("enum : T {};",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_ENUM, 1, 1});
}

TEST_CASE("Illegal underlying type") {
    helpers::test_parser_fail(
        "enum : 4 {A};", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 8});
    helpers::test_parser_fail(
        R"(enum : "e" {A};)",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_IDENTIFIER, 1, 8});
}

TEST_CASE("Empty enum with decl") {
    helpers::test_parser_fail("enum : i64 { const b := fn(&self, a: A): C { c; }; };",
                              syntax::ParserDiagnostic{syntax::ParserError::EMPTY_ENUM, 1, 1});
}

TEST_CASE("Out of order enum") {
    helpers::test_parser_fail("enum : i64 { A = 2l const b := fn(&self, a: A): C { c; }; B = 2l };",
                              syntax::ParserDiagnostic{"Expected token SEMICOLON, found RBRACE",
                                                       syntax::ParserError::UNEXPECTED_TOKEN,
                                                       std::pair{1uz, 66uz}});
}

TEST_CASE("Non-static non-function enum member") {
    helpers::test_parser_fail("enum { A = i32{} const b := 2; };",
                              syntax::ParserDiagnostic{syntax::ParserError::INVALID_MEMBER, 1, 18});
}

} // namespace porpoise::tests

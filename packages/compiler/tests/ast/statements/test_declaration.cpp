#include <initializer_list>
#include <sstream>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "array.hpp"

#include "helpers/ast.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/declaration.hpp"

#include "syntax/operators.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Explicit primitive declaration") {
    helpers::test_stmt(
        "var a: int = 2;",
        ast::DeclStatement{
            syntax::Token{keywords::VAR},
            helpers::make_ident("a"),
            mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                               ast::ExplicitType{
                                                   mods::BASE,
                                                   helpers::make_ident("int"),
                                               }),
            mem::make_box<ast::SignedIntegerExpression>(
                syntax::Token{syntax::TokenType::INT_10, "2"}, 2),
            ast::DeclModifiers::VARIABLE,
        });
}

TEST_CASE("Explicit non-primitive declaration") {
    helpers::test_stmt(
        "var a: Foo = bar;",
        ast::DeclStatement{
            syntax::Token{keywords::VAR},
            helpers::make_ident("a"),
            mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                               ast::ExplicitType{
                                                   mods::BASE,
                                                   helpers::make_ident("Foo"),
                                               }),
            helpers::make_ident("bar"),
            ast::DeclModifiers::VARIABLE,
        });
}

TEST_CASE("Constexpr declaration") {
    helpers::test_stmt(
        "constexpr SIZE := 2uz;",
        ast::DeclStatement{
            syntax::Token{keywords::CONSTEXPR},
            helpers::make_ident("SIZE"),
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
            mem::make_box<ast::USizeIntegerExpression>(
                syntax::Token{syntax::TokenType::UZINT_10, "2uz"}, 2uz),
            ast::DeclModifiers::CONSTEXPR,
        });
}

TEST_CASE("Correct declaration modifiers") {
    const auto test = [](std::initializer_list<syntax::Keyword> modifiers,
                         ast::DeclModifiers                     flags,
                         bool                                   initialized = true) {
        std::stringstream ss;
        for (const auto& keyword : modifiers) { ss << keyword.first << " "; }
        if (initialized) {
            ss << " a := 2;";
            helpers::test_stmt(ss.view(),
                               ast::DeclStatement{
                                   syntax::Token{*modifiers.begin()},
                                   helpers::make_ident("a"),
                                   mem::make_box<ast::TypeExpression>(
                                       syntax::Token{operators::WALRUS}, std::nullopt),
                                   mem::make_box<ast::SignedIntegerExpression>(
                                       syntax::Token{syntax::TokenType::INT_10, "2"}, 2),
                                   flags,
                               });
        } else {
            ss << " a: int;";
            helpers::test_stmt(
                ss.view(),
                ast::DeclStatement{
                    syntax::Token{*modifiers.begin()},
                    helpers::make_ident("a"),
                    mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                                       ast::ExplicitType{
                                                           mods::BASE,
                                                           helpers::make_ident("int"),
                                                       }),
                    std::nullopt,
                    flags,
                });
        }
    };

    // Modifiers are order independent
    test({keywords::EXPORT, keywords::CONST},
         ast::DeclModifiers::EXPORT | ast::DeclModifiers::CONSTANT);
    test({keywords::PUBLIC, keywords::VAR},
         ast::DeclModifiers::PUBLIC | ast::DeclModifiers::VARIABLE);
    test({keywords::VAR, keywords::EXPORT},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::EXPORT);

    // Uninitialized declarations are selective
    test({keywords::VAR}, ast::DeclModifiers::VARIABLE, false);
    test({keywords::EXTERN, keywords::CONST},
         ast::DeclModifiers::EXTERN | ast::DeclModifiers::CONSTANT,
         false);
    test({keywords::VAR, keywords::EXTERN},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::EXTERN,
         false);
    test({keywords::VAR, keywords::PUBLIC},
         ast::DeclModifiers::VARIABLE | ast::DeclModifiers::PUBLIC,
         false);
}

static auto test_decl_fail(std::initializer_list<syntax::Keyword> modifiers,
                           syntax::ParserDiagnostic&&             expected_error,
                           std::string_view                       init = "a := 2;") -> void {
    std::stringstream ss;
    for (const auto& keyword : modifiers) { ss << keyword.first << " "; }
    ss << init;
    helpers::test_parser_fail(ss.view(), std::move(expected_error));
}

TEST_CASE("Mutability restrictions") {
    const std::array contending_mut{keywords::CONSTEXPR, keywords::VAR, keywords::CONST};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::CONSTEXPR, keywords::VAR, keywords::CONST},
                   syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("CONSTEXPR restrictions") {
    const std::array contending_mut{keywords::EXTERN, keywords::CONSTEXPR};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::EXTERN, keywords::CONSTEXPR},
                   syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("ABI/Linkage restrictions") {
    const std::array contending_mut{keywords::EXTERN, keywords::EXPORT};
    for (const auto& mut : array::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second},
                       syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
    }
    test_decl_fail({keywords::EXTERN, keywords::EXPORT},
                   syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_DECL_MODIFIERS, 1, 1});
}

TEST_CASE("Extern requirements") {
    test_decl_fail({keywords::EXTERN, keywords::CONST},
                   syntax::ParserDiagnostic{syntax::ParserError::EXTERN_VALUE_INITIALIZED, 1, 1});
    test_decl_fail({keywords::EXTERN, keywords::VAR},
                   syntax::ParserDiagnostic{syntax::ParserError::EXTERN_VALUE_INITIALIZED, 1, 1});
}

TEST_CASE("Constant requirements") {
    test_decl_fail({keywords::CONST},
                   syntax::ParserDiagnostic{syntax::ParserError::CONST_DECL_MISSING_VALUE, 1, 1},
                   "a: int;");
    test_decl_fail({keywords::CONSTEXPR},
                   syntax::ParserDiagnostic{syntax::ParserError::CONST_DECL_MISSING_VALUE, 1, 1},
                   "a: int;");
}

TEST_CASE("Non-terminated decls") {
    helpers::test_parser_fail(
        "var a: int = 2",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found END", syntax::ParserError::UNEXPECTED_TOKEN, 1, 15});
}

} // namespace porpoise::tests

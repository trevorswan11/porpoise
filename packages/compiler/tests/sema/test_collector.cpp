#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"
#include "helpers/sema.hpp"

#include "syntax/keywords.hpp"
#include "syntax/operators.hpp"

#include "ast/ast.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

namespace helpers {

auto test_illegal_top_level_stmt(std::string_view input, std::string_view stringified) -> void {
    helpers::test_collector_fail(
        input,
        sema::Diagnostic{fmt::format("Cannot have {} at the top level", stringified),
                         sema::Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                         std::pair{1uz, 1uz}});
}

template <typename SymbolicMaker>
auto test_hollow_symbol(sema::Analyzer& analyzer, SymbolicMaker&& maker) -> void {
    auto& registry = analyzer.get_registry();
    CHECK(registry.size() == 2);
    CHECK(registry.get(1).size() == 1);

    const auto         expected_decl = maker();
    const sema::Symbol expected{"b", &expected_decl};
    CHECK(expected == registry.get_from(1, "b"));
}

} // namespace helpers

TEST_CASE("Holistic language examples") {
    const auto test = [](bool is_module) {
        const auto input = fmt::format(R"({}module;
                                        import std;
                                        using Integer = int;
                                        const a: Integer = 1;)",
                                       is_module ? "" : "//");

        helpers::test_collector(
            input,
            is_module,
            std::pair{"std",
                      [is_module]() {
                          ast::ImportStatement import_stmt{
                              syntax::Token{keywords::IMPORT},
                              ast::LibraryImport{helpers::make_ident("std"), {}}};
                          if (is_module) { import_stmt.mark_public(); }
                          return import_stmt;
                      }()},
            std::pair{"Integer",
                      [is_module]() {
                          ast::UsingStatement using_stmt{syntax::Token{keywords::USING},
                                                         helpers::make_ident("Integer"),
                                                         ast::ExplicitType{
                                                             mods::BASE,
                                                             helpers::make_ident("int"),
                                                         }};
                          if (is_module) { using_stmt.mark_public(); }
                          return using_stmt;
                      }()},
            std::pair{
                "a",
                ast::DeclStatement{
                    syntax::Token{keywords::CONST},
                    helpers::make_ident("a"),
                    mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                                       ast::ExplicitType{
                                                           mods::BASE,
                                                           helpers::make_ident("Integer"),
                                                       }),
                    mem::make_box<ast::SignedIntegerExpression>(
                        syntax::Token{syntax::TokenType::INT_10, "1"}, 1),
                    ast::DeclModifiers::CONSTANT,
                }});
    };

    test(true);
    test(false);
}

TEST_CASE("Import aliases correctly used") {
    helpers::test_collector(
        "import a as A; const a := 22;",
        false,
        std::pair{"A",
                  ast::ImportStatement{
                      syntax::Token{keywords::IMPORT},
                      ast::LibraryImport{helpers::make_ident("a"), helpers::make_ident("A")}}},
        std::pair{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::SignedIntegerExpression>(
                    syntax::Token{syntax::TokenType::INT_10, "22"}, 22),
                ast::DeclModifiers::CONSTANT,
            }});
}

TEST_CASE("Struct hollow types") {
    const auto struct_decl = [] {
        return ast::DeclStatement{
            syntax::Token{keywords::VAR},
            helpers::make_ident("b"),
            mem::make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                               ast::ExplicitType{
                                                   mods::BASE,
                                                   helpers::make_ident("Foo"),
                                               }),
            helpers::make_ident("bar"),
            ast::DeclModifiers::VARIABLE,
        };
    };

    auto analyzer = helpers::test_collector(
        "const a := struct { var b: Foo = bar; };",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::StructExpression>(syntax::Token{keywords::STRUCT},
                                                     helpers::make_decls(struct_decl())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::STRUCT, false, 1}});

    helpers::test_hollow_symbol(analyzer, struct_decl);
}

TEST_CASE("Enum hollow types") {
    const auto enumeration = [] { return ast::Enumeration{helpers::make_ident("b"), {}}; };

    auto analyzer = helpers::test_collector(
        "const a := enum {b};",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::EnumExpression>(
                    syntax::Token{keywords::ENUM},
                    std::nullopt,
                    helpers::make_vector<ast::Enumeration>(enumeration())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::ENUM, false, 1}});
    helpers::test_hollow_symbol(analyzer, enumeration);
}

TEST_CASE("Union hollow types") {
    const auto field = [] {
        return ast::UnionField{helpers::make_ident("b"),
                               ast::ExplicitType{mods::BASE, helpers::make_ident("int")}};
    };

    auto analyzer = helpers::test_collector(
        "const a := union { b: int };",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::UnionExpression>(syntax::Token{keywords::UNION},
                                                    helpers::make_vector<ast::UnionField>(field())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::UNION, false, 1}});
    helpers::test_hollow_symbol(analyzer, field);
}

TEST_CASE("Duplicate module declaration") {
    helpers::test_collector_fail("module; module;",
                                 sema::Diagnostic{"Only one module statement is allowed per file",
                                                  sema::Error::DUPLICATE_MODULE_STATEMENT,
                                                  std::pair{1uz, 9uz}});
}

TEST_CASE("Illegal module location") {
    helpers::test_collector_fail(
        "const a := 2; module;",
        sema::Diagnostic{"Module indicator must be first statement of file",
                         sema::Error::ILLEGAL_MODULE_STATEMENT_LOCATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Duplicate identifiers") {
    helpers::test_collector_fail(
        "const a := 2; import a;",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: [1, 1]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Illegal top-level statement") {
    helpers::test_illegal_top_level_stmt("{}", "block");
    helpers::test_illegal_top_level_stmt("defer 2;", "defer");
    helpers::test_illegal_top_level_stmt("_ = 2;", "discard");
    helpers::test_illegal_top_level_stmt("2;", "expression");
    helpers::test_illegal_top_level_stmt("return 2;", "jump");
}

TEST_CASE("Shadowing declarations") {
    helpers::test_collector_fail(
        "const a := struct { const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 21uz}});

    helpers::test_collector_fail(
        "const a := enum {a};",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 18uz}});

    helpers::test_collector_fail(
        "const a := union { a: int };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 20uz}});
}

} // namespace porpoise::tests

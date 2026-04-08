#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"
#include "helpers/sema.hpp"

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

template <typename SymbolicMaker> struct HollowSymbol {
    std::string_view name;
    SymbolicMaker    maker;
};

// Shallowly checks the symbols in the inner scope of a statement
template <typename... HollowSymbols>
auto test_hollow_symbols(sema::Analyzer& analyzer, HollowSymbols&&... symbol) -> void {
    auto& registry = analyzer.get_registry();
    CHECK(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(symbol));

    (..., [&] {
        const auto         expected_decl = symbol.maker();
        const sema::Symbol expected{symbol.name, &expected_decl};
        CHECK(expected == registry.get_from(1, symbol.name));
    }());
}

} // namespace helpers

TEST_CASE("Holistic language examples") {
    const auto test = [](bool is_module) {
        const auto input = fmt::format(R"({}module;
                                        import std;
                                        using Integer = i32;
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
                                                             helpers::make_ident("i32"),
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
                    helpers::make_number<ast::I32Expression>("1"),
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
                helpers::make_number<ast::I32Expression>("22"),
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

    helpers::test_hollow_symbols(analyzer, helpers::HollowSymbol{"b", struct_decl});
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
                    helpers::make_vector<ast::Enumeration>(enumeration()),
                    helpers::make_decls()),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::ENUM, false, 1}});
    helpers::test_hollow_symbols(analyzer, helpers::HollowSymbol{"b", enumeration});
}

TEST_CASE("Enum hollow types with member") {
    const auto enumeration = [] { return ast::Enumeration{helpers::make_ident("b"), {}}; };
    const auto member      = [] {
        return ast::DeclStatement{
            syntax::Token{keywords::STATIC},
            helpers::make_ident("c"),
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
            helpers::make_number<ast::I32Expression>("2"),
            ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
        };
    };

    auto analyzer = helpers::test_collector(
        "const a := enum {b, static const c := 2; };",
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
                    helpers::make_vector<ast::Enumeration>(enumeration()),
                    helpers::make_decls(member())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::ENUM, false, 1}});

    helpers::test_hollow_symbols(
        analyzer, helpers::HollowSymbol{"b", enumeration}, helpers::HollowSymbol{"c", member});
}

TEST_CASE("Union hollow types") {
    const auto field = [] {
        return ast::UnionField{helpers::make_ident("b"),
                               ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}};
    };

    auto analyzer = helpers::test_collector(
        "const a := union { b: i32 };",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::UnionExpression>(syntax::Token{keywords::UNION},
                                                    helpers::make_vector<ast::UnionField>(field()),
                                                    helpers::make_decls()),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::UNION, false, 1}});
    helpers::test_hollow_symbols(analyzer, helpers::HollowSymbol{"b", field});
}

TEST_CASE("Union hollow types with member") {
    const auto field = [] {
        return ast::UnionField{helpers::make_ident("b"),
                               ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}};
    };

    const auto member = [] {
        return ast::DeclStatement{
            syntax::Token{keywords::STATIC},
            helpers::make_ident("c"),
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
            helpers::make_number<ast::I32Expression>("2"),
            ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
        };
    };

    auto analyzer = helpers::test_collector(
        "const a := union { b: i32, static const c := 2; };",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::UnionExpression>(syntax::Token{keywords::UNION},
                                                    helpers::make_vector<ast::UnionField>(field()),
                                                    helpers::make_decls(member())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::UNION, false, 1}});

    helpers::test_hollow_symbols(
        analyzer, helpers::HollowSymbol{"b", field}, helpers::HollowSymbol{"c", member});
}

TEST_CASE("Function hollow types") {
    const auto function_block_decl = [] {
        return ast::DeclStatement{
            syntax::Token{keywords::CONST},
            helpers::make_ident("b"),
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
            helpers::make_ident("bar"),
            ast::DeclModifiers::CONSTANT,
        };
    };

    auto analyzer = helpers::test_collector(
        "const a := fn(&self, c: type): void { const b := bar; };",
        false,
        std::tuple{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONST},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, std::nullopt),
                mem::make_box<ast::FunctionExpression>(
                    syntax::Token{keywords::FN},
                    ast::SelfParameter{mods::REF, helpers::make_ident("self")},
                    helpers::make_parameters(ast::FunctionParameter{
                        helpers::make_ident("c"), {mods::BASE, helpers::make_ident("type")}}),
                    false,
                    ast::ExplicitType{mods::BASE, helpers::make_ident("void")},
                    helpers::make_block_stmt(function_block_decl())),
                ast::DeclModifiers::CONSTANT,
            },
            sema::types::Key{sema::TypeKind::FUNCTION, false, 1}},
        std::tuple{"self", ast::SelfParameter{mods::REF, helpers::make_ident("self")}},
        std::tuple{"c",
                   ast::FunctionParameter{helpers::make_ident("c"),
                                          {mods::BASE, helpers::make_ident("type")}}});
    helpers::test_hollow_symbols(analyzer, helpers::HollowSymbol{"b", function_block_decl});
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
        "const a := enum {b static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 20uz}});

    helpers::test_collector_fail(
        "const a := union { a: i32 };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 20uz}});

    helpers::test_collector_fail(
        "const a := union { b: i32 static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 27uz}});
}

namespace {

using namespace std::string_view_literals;

constexpr auto RESTRICTED_INPUTS = std::array{
    std::pair{"a := fn(): void {};"sv, "function"sv},
    std::pair{"a := struct { const b := 2; };"sv, "struct"sv},
    std::pair{"a := enum { A };"sv, "enum"sv},
    std::pair{"a := union { b: bool };"sv, "union"sv},
};

TEST_CASE("Restricted non-const top level declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("var {}", input),
            sema::Diagnostic{
                fmt::format("Top level {}s must be marked const at the top level", desc),
                sema::Error::ILLEGAL_NON_CONST_STATEMENT,
                std::pair{1uz, 1uz}});
    }
}

TEST_CASE("Restricted non-const top level struct declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("const S := struct {{ var {} }};", input),
            sema::Diagnostic{
                fmt::format("Top level {}s must be marked const at the top level", desc),
                sema::Error::ILLEGAL_NON_CONST_STATEMENT,
                std::pair{1uz, 21uz}});
    }
}

TEST_CASE("Redundant constexpr usage on top level declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("constexpr {}", input),
            sema::Diagnostic{fmt::format("Top level {}s are implicitly compile time known", desc),
                             sema::Error::REDUNDANT_CONSTEXPR,
                             std::pair{1uz, 1uz}});
    }
}

} // namespace

TEST_CASE("Function self param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'f'. Previous declaration here: [1, 1]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Function basic param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'f'. Previous declaration here: [1, 1]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 15uz}});
}

TEST_CASE("Function local param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(a, a: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: [1, 15]",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 18uz}});
}

TEST_CASE("Function block shadowing") {
    helpers::test_collector_fail(
        "const f := fn(): void { var f := 3; };",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: [1, 1]",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 25uz}});
}

} // namespace porpoise::tests

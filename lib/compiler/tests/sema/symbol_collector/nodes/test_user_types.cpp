#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;

TEST_CASE("Struct hollow types") {
    const sema::types::Key key{sema::TypeKind::STRUCT, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := struct { const foo := bar; };",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::StructExpression>(
                    syntax::Token{keywords::STRUCT}, helpers::make_decls(helpers::foo_bar_decl())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key});

    helpers::test_hollow_symbols(analyzer, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Enum hollow types") {
    const auto enumeration = [] { return ast::Enumeration{helpers::make_ident("b"), {}}; };

    const sema::types::Key key{sema::TypeKind::ENUM, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := enum {b};",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::EnumExpression>(
                    syntax::Token{keywords::ENUM},
                    nullptr,
                    helpers::make_vector<ast::Enumeration>(enumeration()),
                    helpers::make_decls()),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key});

    helpers::test_hollow_symbols(analyzer,
                                 helpers::TableEntry<ast::Enumeration>{"b", enumeration()});
}

TEST_CASE("Enum hollow types with member") {
    const auto enumeration = [] { return ast::Enumeration{helpers::make_ident("b"), {}}; };
    const auto member      = [] {
        return ast::DeclStatement{
            syntax::Token{keywords::STATIC},
            helpers::make_ident("c"),
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
            helpers::make_primitive<ast::I32Expression, true>("2"),
            ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
        };
    };

    const sema::types::Key key{sema::TypeKind::ENUM, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := enum {b, static const c := 2; };",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::EnumExpression>(
                    syntax::Token{keywords::ENUM},
                    nullptr,
                    helpers::make_vector<ast::Enumeration>(enumeration()),
                    helpers::make_decls(member())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key});

    helpers::test_hollow_symbols(analyzer,
                                 helpers::TableEntry<ast::Enumeration>{"b", enumeration()},
                                 helpers::TableEntry{"c", member()});
}

TEST_CASE("Union hollow types") {
    const auto field = [] {
        return ast::UnionField{helpers::make_ident("b"),
                               ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}};
    };

    const sema::types::Key key{sema::TypeKind::UNION, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := union { b: i32 };",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::UnionExpression>(
                    syntax::Token{keywords::UNION},
                    helpers::make_vector<ast::UnionField>(field()),
                    helpers::make_decls()),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key});

    helpers::test_hollow_symbols(analyzer, helpers::TableEntry{"b", field()});
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
            mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
            helpers::make_primitive<ast::I32Expression, true>("2"),
            ast::DeclModifiers::STATIC | ast::DeclModifiers::CONSTANT,
        };
    };

    const sema::types::Key key{sema::TypeKind::UNION, false, 1};
    auto                   analyzer = helpers::test_collector(
        "const a := union { b: i32, static const c := 2; };",
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::UnionExpression>(
                    syntax::Token{keywords::UNION},
                    helpers::make_vector<ast::UnionField>(field()),
                    helpers::make_decls(member())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key});

    helpers::test_hollow_symbols(
        analyzer, helpers::TableEntry{"b", field()}, helpers::TableEntry{"c", member()});
}

TEST_CASE("Shadowing member/field declarations") {
    helpers::test_collector_fail(
        "const a := struct { const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 21uz}});

    helpers::test_collector_fail(
        "const a := enum {a};",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 18uz}});

    helpers::test_collector_fail(
        "const a := enum {b static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 20uz}});

    helpers::test_collector_fail(
        "const a := union { a: i32 };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 20uz}});

    helpers::test_collector_fail(
        "const a := union { b: i32 static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 27uz}});
}

} // namespace porpoise::tests

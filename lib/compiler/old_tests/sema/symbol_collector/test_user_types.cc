#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

namespace keywords  = syntax::keywords;
namespace operators = syntax::operators;
namespace mods      = helpers::type_modifiers;
namespace mut       = helpers::mut;

TEST_CASE("Struct hollow types") {
    const sema::types::Key key{sema::TypeKind::STRUCT, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := struct { var foo := bar; };",
        {},
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::StructExpression>(
                    syntax::Token{keywords::STRUCT},
                    helpers::make_members(helpers::foo_bar_decl(false))),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::TYPE});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"foo", helpers::foo_bar_decl(false)});
}

TEST_CASE("Enum hollow types") {
    const auto enumeration = [] { return ast::Enumeration{helpers::make_ident("b"), {}}; };

    const sema::types::Key key{sema::TypeKind::ENUM, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := enum {b};",
        {},
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
                    helpers::make_members()),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::TYPE});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry<ast::Enumeration>{"b", enumeration()});
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

    const sema::types::Key key{sema::TypeKind::ENUM, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := enum {b, static const c := 2; };",
        {},
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
                    helpers::make_members(member())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::TYPE});

    helpers::test_hollow_symbols(ctx,
                                 helpers::TableEntry<ast::Enumeration>{"b", enumeration()},
                                 helpers::TableEntry{"c", member()});
}

TEST_CASE("Union hollow types") {
    const auto field = [] {
        return ast::UnionField{helpers::make_ident("b"),
                               ast::ExplicitType{mods::BASE, helpers::make_ident("i32")}};
    };

    const sema::types::Key key{sema::TypeKind::UNION, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := union { b: i32 };",
        {},
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::UnionExpression>(
                    syntax::Token{keywords::UNION},
                    helpers::make_vector<ast::UnionField>(field()),
                    helpers::make_members()),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::TYPE});

    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"b", field()});
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

    const sema::types::Key key{sema::TypeKind::UNION, mut::IMMUTABLE, 1};
    auto                   ctx = helpers::test_collector(
        "const a := union { b: i32, static const c := 2; };",
        {},
        helpers::TableEntry<ast::DeclStatement>{
            "a",
            ast::DeclStatement{
                syntax::Token{keywords::CONSTANT},
                helpers::make_ident("a"),
                mem::make_box<ast::TypeExpression>(syntax::Token{operators::WALRUS}, opt::none),
                mem::make_nullable_box<ast::UnionExpression>(
                    syntax::Token{keywords::UNION},
                    helpers::make_vector<ast::UnionField>(field()),
                    helpers::make_members(member())),
                ast::DeclModifiers::CONSTANT,
            },
            opt::none,
            key,
            key,
            sema::SymbolKind::TYPE});

    helpers::test_hollow_symbols(
        ctx, helpers::TableEntry{"b", field()}, helpers::TableEntry{"c", member()});
}

TEST_CASE("Public using query") {
    const auto test = [](bool is_public) {
        const auto input = fmt::format("{} using I = i32;", is_public ? "pub" : "");

        auto ctx = helpers::test_collector(
            input,
            {},
            helpers::TableEntry<ast::UsingStatement>{
                "I",
                ast::UsingStatement{syntax::Token{is_public ? keywords::PUBLIC : keywords::USING},
                                    helpers::make_ident("I"),
                                    ast::ExplicitType{
                                        mods::BASE,
                                        helpers::make_ident("i32"),
                                    }},
                opt::none,
                opt::none,
                sema::SymbolKind::TYPE});

        auto&       table     = ctx.analyzer.get_table(*ctx.root_mod->root_table_idx);
        const auto& int_alias = table.get_opt("I");
        REQUIRE(int_alias);
        CHECK(int_alias->is_public() == is_public);
    };

    test(true);
    test(false);
}

} // namespace porpoise::tests

#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "types.hh"

namespace porpoise::tests {

namespace mut = sema::types::mut;

namespace {

auto test_user_type(std::string_view input, sema::TypeKind kind, usize expected_reg_count)
    -> helpers::SemaTestContext {
    auto [ctx, idx]      = helpers::collect_and_check(input);
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == expected_reg_count);

    const auto& symbol_a = helpers::unwrap(registry.get_from_opt(idx, "a"));
    CHECK(symbol_a.get_kind_opt() == sema::SymbolKind::TYPE);
    const auto& expected_type = ctx.analyzer.get_pool()[{kind, mut::CONSTANT, 1}];

    const auto  symbolic_node = helpers::unwrap(symbol_a.as_opt<sema::symbols::Node>());
    const auto& a_decl =
        helpers::unwrap(ctx.root_mod->ast.get_as_opt<ast::DeclStatement>(*symbolic_node));
    auto& actual_type = helpers::unwrap(ctx.root_mod->get_sema_type_opt(a_decl.ident));
    CHECK(actual_type.get_symbol_table_idx_opt() == idx + 1);
    CHECK(&actual_type == &expected_type);

    const auto& value_type = helpers::unwrap(ctx.root_mod->get_sema_type_opt(*a_decl.value));
    CHECK(&expected_type == &value_type);

    return std::move(ctx);
}

} // namespace

TEST_CASE("Struct hollow types") {
    auto ctx = test_user_type("const a := struct { var foo := bar; };", sema::TypeKind::STRUCT, 2);
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Enum hollow types") {
    auto ctx =
        test_user_type("const a := enum {b, static const foo := bar; };", sema::TypeKind::ENUM, 2);
    const auto& enum_table  = helpers::unwrap(ctx.analyzer.get_table_opt(1));
    const auto& enumeration = helpers::unwrap(enum_table.get_opt("b"));
    REQUIRE(enumeration.as_opt<sema::symbols::Enumeration>());
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Union hollow types") {
    auto ctx = test_user_type(
        "const a := union { b: i32, static const foo := bar; };", sema::TypeKind::UNION, 2);
    const auto& union_table = helpers::unwrap(ctx.analyzer.get_table_opt(1));
    const auto& field       = helpers::unwrap(union_table.get_opt("b"));
    REQUIRE(field.as_opt<sema::symbols::UnionField>());
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Public using query") {
    auto [ctx, idx]       = helpers::collect_and_check("pub using I = i32;");
    auto&       table     = helpers::unwrap(ctx.analyzer.get_table_opt(idx));
    const auto& int_alias = helpers::unwrap(table.get_opt("I"));
    CHECK(int_alias.get_kind_opt() == sema::SymbolKind::TYPE);
    CHECK(int_alias.is_public(*ctx.root_mod));
}

TEST_CASE("Shadowing member/field declarations") {
    helpers::test_collector_fail(
        "const a := struct { var a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 20uz}});

    helpers::test_collector_fail(
        "const a := enum {a};",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 17uz}});

    helpers::test_collector_fail(
        "const a := enum {b static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 19uz}});

    helpers::test_collector_fail(
        "const a := union { a: i32 };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 19uz}});

    helpers::test_collector_fail(
        "const a := union { b: i32 static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 26uz}});
}

} // namespace porpoise::tests

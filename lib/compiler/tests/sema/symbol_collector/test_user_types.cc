#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
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

    const auto& symbol_a = registry.get_from_opt(idx, "a");
    REQUIRE(symbol_a);
    REQUIRE(symbol_a->has_kind());
    CHECK(symbol_a->get_kind() == sema::SymbolKind::TYPE);

    auto&       pool          = ctx.analyzer.get_pool();
    const auto& expected_type = pool[{kind, mut::CONSTANT, 1}];
    REQUIRE(symbol_a->has_type());
    CHECK(&symbol_a->get_type() == &expected_type);
    CHECK(expected_type.get_symbol_table_idx() == idx + 1);

    REQUIRE(symbol_a->is_symbolic_node());
    const auto& a_decl =
        ctx.root_mod->ast.get_as<ast::DeclStatement>(*symbol_a->get_symbolic_node());
    REQUIRE(ctx.root_mod->has_sema_type(**a_decl.value));
    CHECK(&expected_type == &ctx.root_mod->get_sema_type(**a_decl.value));

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
    const auto& enum_table  = ctx.analyzer.get_table(1);
    const auto& enumeration = enum_table.get_opt("b");
    REQUIRE(enumeration->is_enumeration());
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Union hollow types") {
    auto ctx = test_user_type(
        "const a := union { b: i32, static const foo := bar; };", sema::TypeKind::UNION, 2);
    const auto& union_table = ctx.analyzer.get_table(1);
    const auto& field       = union_table.get_opt("b");
    REQUIRE(field->is_union_field());
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Public using query") {
    auto [ctx, idx]       = helpers::collect_and_check("pub using I = i32;");
    auto&       table     = ctx.analyzer.get_table(idx);
    const auto& int_alias = table.get_opt("I");
    REQUIRE(int_alias);
    REQUIRE(int_alias->has_kind());
    CHECK(int_alias->get_kind() == sema::SymbolKind::TYPE);
    CHECK(int_alias->is_public(*ctx.root_mod));
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

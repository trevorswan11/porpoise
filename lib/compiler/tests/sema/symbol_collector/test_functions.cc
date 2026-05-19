#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

namespace porpoise::tests {

namespace mut = sema::types::mut;

TEST_CASE("Function hollow types") {
    auto [ctx, idx] =
        helpers::collect_and_check("const a := fn(&self, c: type): void { const foo := bar; };");
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    const auto& symbol_a = registry.get_from(idx, "a");
    REQUIRE(symbol_a.has_kind());
    CHECK(symbol_a.get_kind() == sema::SymbolKind::CALLABLE);

    const auto symbolic_node = symbol_a.as_opt<sema::symbols::Node>();
    REQUIRE(symbolic_node);
    const auto& decl_a = ctx.root_mod->ast.get_as<ast::DeclStatement>(*symbolic_node);
    REQUIRE(ctx.root_mod->has_sema_type(**decl_a.value));
    const auto& fn_type = ctx.root_mod->get_sema_type(**decl_a.value);

    auto& pool = ctx.analyzer.get_pool();
    CHECK(&fn_type == &pool[{sema::TypeKind::FUNCTION, mut::CONSTANT, 1}]);

    const auto& self_param = registry.get_from(1, "self");
    REQUIRE(self_param.as_opt<sema::symbols::SelfParameter>());
    const auto& c_param = registry.get_from(1, "c");
    REQUIRE(c_param.as_opt<sema::symbols::Parameter>());
    ctx.test_common_decl_collection(1);
}

TEST_CASE("Well-placed function control-flow statements") {
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): void { return; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { return 0; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { defer a = 2; };");
}

TEST_CASE("Constexpr function declaration") {
    helpers::collect_and_check("pub constexpr work := fn(): i32 { return 1; };");
}

TEST_CASE("Defer statements respect identifier collection rules") {
    helpers::test_collector_fail(
        "pub const main := fn(args: [][:0]u8): i32 { defer { var main: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'main'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 52uz}});
}

TEST_CASE("Function basic param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f: bool): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function self param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function local param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(a, a: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: 1:15",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 17uz}});
}

TEST_CASE("Function block shadowing") {
    helpers::test_collector_fail(
        "const f := fn(): void { var f := 3; };",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 24uz}});
}

} // namespace porpoise::tests

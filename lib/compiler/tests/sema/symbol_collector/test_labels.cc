#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "types.hh"

namespace porpoise::tests {

namespace mut = sema::types::mut;

namespace {

auto collect_and_validate_label(std::string_view input, usize expected_size) -> void {
    auto [ctx, idx] = helpers::collect_and_check(input);

    auto&       analyzer = ctx.analyzer;
    const auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == expected_size);

    const auto& top_table = registry.get(idx);
    CHECK(top_table.has("a"));
    CHECK_FALSE(top_table.has("blk"));

    const auto  blk_idx     = idx + 1;
    const auto& label_table = registry.get(blk_idx);
    REQUIRE(label_table.has("blk"));
    const auto& blk_symbol = label_table.get("blk");
    CHECK(blk_symbol.get_kind() == sema::SymbolKind::LABEL);
    REQUIRE(blk_symbol.is_symbolic_node());

    const auto& label_type = ctx.root_mod->get_sema_type(*blk_symbol.get_symbolic_node());
    CHECK(label_type.get_symbol_table_idx() == blk_idx);
    CHECK(&label_type == &ctx.analyzer.get_pool()[{sema::TypeKind::LABEL, mut::CONSTANT, blk_idx}]);
}

} // namespace

TEST_CASE("Label collection") {
    collect_and_validate_label("const a := blk: for (0..5) |i| { const foo := bar; };", 3);
    collect_and_validate_label("const a := blk: { if (b) { break :blk c; } else break :blk 5; };",
                               4);
}

TEST_CASE("Label redeclaration") {
    helpers::test_collector_fail(
        "const a := a: {};",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 12uz}});
}

TEST_CASE("Label shadowing") {
    helpers::test_collector_fail(
        "const a := blk: { var blk: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'blk'. Previous declaration here: 1:15",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 18uz}});
}

} // namespace porpoise::tests

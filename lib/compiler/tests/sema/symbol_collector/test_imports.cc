#include <sstream>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "types.hh"

namespace porpoise::tests {

using MockFile = helpers::MockFile;

TEST_CASE("Import aliases correctly used") {
    auto [ctx, idx] = helpers::collect_and_check(
        R"(import foo as A; import "f.porp" as F; const foo := bar;)",
        helpers::make_vector<MockFile>(MockFile{"foo.porp", "const foo := bar;", "foo"},
                                       MockFile{"f.porp", "const foo := bar;"}));

    const auto& registry = ctx->analyzer.get_registry();
    REQUIRE(registry.size() == 3);
    ctx->test_common_decl_collection(idx);

    const auto test_import_inner = [&](std::string_view import_name, usize inner_idx) {
        const auto [sym, sym_data, type, type_data] =
            ctx->get_full_type_sym_info<sema::symbols::Node, sema::types::Module>(import_name, idx);
        CHECK(sym.get_kind_opt() == sema::SymbolKind::MODULE);

        CHECK(type == ctx->get_type(sema::TypeKind::MODULE, inner_idx));
        helpers::test_common_decl_collection(registry, type_data.imported, inner_idx);
    };

    test_import_inner("A", 1);
    test_import_inner("F", 2);
}

TEST_CASE("Public import query") {
    auto [ctx, idx] = helpers::collect_and_check(
        "pub import std;",
        helpers::make_vector<MockFile>(MockFile{"std.porp", "var a: i32;", "std"}));

    auto&       table      = helpers::unwrap(ctx->analyzer.get_table_opt(idx));
    const auto& std_import = helpers::unwrap(table.get_opt("std"));
    CHECK(std_import.is_public(*ctx->root_mod));
}

constexpr std::string_view a_porp{R"(pub import "b.porp" as b;)"};
constexpr std::string_view b_porp{R"(pub import "a.porp" as a;)"};

TEST_CASE("Circular imports") {
    auto [ctx, _] = helpers::collect_and_check(
        a_porp, helpers::make_vector<MockFile>(MockFile{"b.porp", b_porp}));
    const auto& registry = ctx->analyzer.get_registry();
    REQUIRE(registry.size() == 2);

    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "a"));
}

constexpr std::string_view importer_porp{R"(
    import "a.porp" as a;
    import "b.porp" as b;
)"};

constexpr std::string_view diamond{R"(import std;)"};
constexpr std::string_view std_porp{R"(pub import "io.porp" as io;)"};

TEST_CASE("Diamond dependencies") {
    auto [ctx, _] = helpers::collect_and_check(
        importer_porp,
        helpers::make_vector<MockFile>(MockFile{"a.porp", diamond},
                                       MockFile{"b.porp", diamond},
                                       MockFile{"std.porp", std_porp, "std"}));
    const auto& registry = ctx->analyzer.get_registry();
    REQUIRE(registry.size() == 4);

    CHECK(registry.get_from_opt(0, "a"));
    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "std"));
    CHECK(registry.get_from_opt(2, "io"));
    CHECK(registry.get_from_opt(3, "std"));
}

TEST_CASE("Self import") {
    helpers::SemaTestContext ctx{{}, "self.porp", R"(import "self.porp" as self;)"};
    REQUIRE_FALSE(ctx.root_mod->has_parser_diagnostics());
    ctx.analyzer.collect_symbols(*ctx.root_mod);
    REQUIRE_FALSE(ctx.root_mod->has_sema_diagnostics());
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 1);
    CHECK(registry.get_from_opt(0, "self"));
}

TEST_CASE("Unknown file module") {
    std::stringstream ss;
    auto ctx = helpers::analyze(helpers::TEST_FILENAME, ss, R"(import "a.porp" as a;)");
    REQUIRE(ctx->root_mod->has_sema_diagnostics());

    constexpr std::string_view expected{
        R"(test.porp:1:8: error: Could not find path 'a.porp' in virtual file system
    import "a.porp" as a;
           ^
)"};
    CHECK(ss.view() == expected);
}

} // namespace porpoise::tests

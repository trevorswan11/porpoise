#include <catch2/catch_test_macros.hpp>
#include <tuple>

#include "helpers/sema.hh"

#include "sema/symbol.hh"

namespace porpoise::tests {

namespace {

[[nodiscard]] auto setup_basic_import() {
    mod::MemoryLoader loader;
    loader.add(helpers::TEST_FILENAME, std::string{"import a;"});
    mod::ModuleManager manager{loader};
    REQUIRE(manager.add_library_module("a", std::string{helpers::TEST_FILENAME}));
    const auto mod_result = manager.try_get_library_module("a");
    const auto module     = *mod_result;
    REQUIRE_FALSE(module->is_errored());
    const auto import_handle = ast::ImportHandle{module->ast[0]};

    return std::tuple{module, import_handle, std::pair{std::move(loader), std::move(manager)}};
}

} // namespace

TEST_CASE("Basic table operations") {
    const auto [module, import_handle, memory] = setup_basic_import();

    sema::SymbolTable table;
    CHECK(table.empty());
    CHECK(table.insert<sema::SymbolicImport>("a", *module, import_handle, opt::none));
    CHECK(table.size() == 1);
    CHECK_FALSE(table.empty());

    CHECK(table.has("a"));
    const auto& retrieved = table.get_opt("a");
    CHECK(retrieved);
    CHECK(retrieved->get_name() == "a");
    CHECK_FALSE(retrieved->has_type());
    CHECK(retrieved->is_import_stmt());
}

TEST_CASE("Duplicate table inserts") {
    const auto [module, import_handle, memory] = setup_basic_import();
    sema::SymbolTable table;
    CHECK(table.insert<sema::SymbolicImport>("a", *module, import_handle, opt::none));
    CHECK_FALSE(table.insert<sema::SymbolicImport>("a", *module, import_handle, opt::none));
    CHECK(table.size() == 1);
}

TEST_CASE("Illegal registry insert") {
    const auto [module, import_handle, memory] = setup_basic_import();
    sema::SymbolTableRegistry registry;
    const auto                result =
        registry.insert_into<sema::SymbolicImport>(0, *module, "a", import_handle, opt::none);

    CHECK_FALSE(result);
    CHECK(result.error() == sema::Diagnostic{sema::Error::INVALID_TABLE_IDX});
}

TEST_CASE("Safety checked registry operations") {
    sema::SymbolTableRegistry registry;
    CHECK_THROWS(registry.get(1));
    CHECK_FALSE(registry.get_opt(1));
    CHECK_THROWS(registry.get_from(1, "a"));
    CHECK_FALSE(registry.get_from_opt(1, "a"));
}

} // namespace porpoise::tests

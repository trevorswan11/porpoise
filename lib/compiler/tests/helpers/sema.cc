#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "ast/statement.hh"
#include "helpers/common.hh"
#include "helpers/sema.hh"
#include "module/memory_loader.hh"
#include "module/module.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"

#include "memory.hh"
#include "types.hh"

namespace porpoise::tests::helpers {

auto test_common_decl_collection(const sema::SymbolTableRegistry& registry,
                                 const mod::Module&               module,
                                 usize                            idx,
                                 std::string_view                 name) -> void {
    const auto& symbol = helpers::unwrap(registry.get_from_opt(idx, name));
    const auto& node   = helpers::unwrap(symbol.as_opt<sema::symbols::Node>());
    CHECK_FALSE(symbol.is_public(module));
    CHECK(node.is<ast::DeclStatement>());
}

SemaTestContext::SemaTestContext(const std::vector<MockFile>& imports,
                                 const std::filesystem::path& root_path,
                                 std::string_view             input,
                                 std::ostream&                error_stream)
    : loader{mem::make_box<mod::MemoryLoader>()}, manager{*loader},
      analyzer{manager, error_stream, false}, root_mod{[&] {
          loader->add(root_path, std::string{input});
          for (const auto& mock : imports) {
              loader->add(mock.path, std::string{mock.source});
              if (mock.name) { REQUIRE(manager.add_library_module(*mock.name, mock.path)); }
          }

          return helpers::unwrap(manager.try_get_file_module(root_path));
      }()} {}

auto SemaTestContext::test_common_decl_collection(usize idx, std::string_view name) -> void {
    const auto& registry = analyzer.get_registry();
    helpers::test_common_decl_collection(registry, *root_mod, idx, name);
}

auto collect(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<mem::Box<SemaTestContext>, usize> {
    auto ctx = mem::make_box<SemaTestContext>(imports, TEST_FILENAME, input);
    REQUIRE_FALSE(ctx->root_mod->has_parser_diagnostics());
    ctx->analyzer.collect_symbols(*ctx->root_mod);

    REQUIRE(ctx->root_mod->root_table_idx);
    usize idx = *ctx->root_mod->root_table_idx;
    return {std::move(ctx), idx};
}

auto collect_and_check(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<mem::Box<SemaTestContext>, usize> {
    auto [ctx, idx] = collect(input, imports);
    if (ctx->root_mod->has_sema_diagnostics()) {
        check_errors<sema::Diagnostic>(ctx->root_mod->get_sema_diagnostics());
    }
    return {std::move(ctx), idx};
}

auto resolve(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<mem::Box<SemaTestContext>, usize> {
    auto [ctx, idx] = collect(input, imports);
    ctx->analyzer.resolve_types(*ctx->root_mod);
    return {std::move(ctx), idx};
}

auto resolve_and_check(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<mem::Box<SemaTestContext>, usize> {
    auto [ctx, idx] = resolve(input, imports);
    if (ctx->root_mod->has_sema_diagnostics()) {
        check_errors<sema::Diagnostic>(ctx->root_mod->get_sema_diagnostics());
    }

    // A successful resolution should resolve every single symbol
    for (usize i = 0; const auto& table : ctx->analyzer.get_registry()) {
        for (const auto& [name, symbol] : table) {
            CHECK(symbol.get_status() == sema::SymbolStatus::RESOLVED);
            if (symbol.get_status() != sema::SymbolStatus::RESOLVED) {
                FAIL(name << "  was not resolved in table " << i);
            }
        }
        i++;
    }
    return {std::move(ctx), idx};
}

} // namespace porpoise::tests::helpers

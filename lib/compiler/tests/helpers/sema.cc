#include "helpers/sema.hh"

namespace porpoise::tests::helpers {

auto test_common_decl_collection(const sema::SymbolTableRegistry& registry,
                                 const mod::Module&               module,
                                 usize                            idx,
                                 std::string_view                 name) -> void {
    const auto& symbol = registry.get_from_opt(idx, name);
    REQUIRE(symbol);
    REQUIRE(symbol->is_symbolic_node());
    CHECK_FALSE(symbol->is_public(module));

    const auto& node = symbol->get_symbolic_node();
    CHECK(node->is<ast::DeclStatement>());
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
              if (mock.name) {
                  REQUIRE(manager.add_library_module(std::string{*mock.name}, mock.path));
              }
          }

          auto test_mod_result = manager.try_get_file_module(root_path);
          REQUIRE(test_mod_result);
          return *test_mod_result;
      }()} {}

auto SemaTestContext::test_common_decl_collection(usize idx, std::string_view name) -> void {
    const auto& registry = analyzer.get_registry();
    helpers::test_common_decl_collection(registry, *root_mod, idx, name);
}

auto collect(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<SemaTestContext, usize> {
    SemaTestContext ctx{imports, TEST_FILENAME, input};
    auto            test_mod = ctx.root_mod;
    REQUIRE_FALSE(test_mod->has_parser_diagnostics());
    ctx.analyzer.collect_symbols(*test_mod);

    return {std::move(ctx), *test_mod->root_table_idx};
}

auto collect_and_check(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<SemaTestContext, usize> {
    auto [ctx, idx] = collect(input, imports);
    if (ctx.root_mod->has_sema_diagnostics()) {
        check_errors<sema::Diagnostic>(ctx.root_mod->get_sema_diagnostics());
    }
    return {std::move(ctx), idx};
}

} // namespace porpoise::tests::helpers

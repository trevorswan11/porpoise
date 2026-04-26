#include "helpers/sema.hpp"

namespace porpoise::tests::helpers {

SemaTestContext::SemaTestContext(const std::vector<MockFile>& imports,
                                 const std::filesystem::path& root_path,
                                 std::string_view             input)
    : loader{mem::make_box<sema::mod::MemoryLoader>()}, manager{*loader}, analyzer{manager},
      root_mod{[&] {
          loader->add(root_path, std::string{input});
          for (const auto& mock : imports) {
              loader->add(mock.path, std::string{mock.source});
              if (mock.name) {
                  REQUIRE(manager.add_porpoise_module(std::string{*mock.name}, mock.path));
              }
          }

          auto test_mod_result = manager.try_get_file_module(root_path);
          REQUIRE(test_mod_result);
          return *test_mod_result;
      }()} {}

auto collect(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<SemaTestContext, usize> {
    SemaTestContext ctx{imports, test_file, input};
    auto            test_mod = ctx.root_mod;
    REQUIRE_FALSE(test_mod->has_parser_diagnostics());
    ctx.analyzer.collect_symbols(*test_mod);

    return {std::move(ctx), test_mod->root_table_idx};
}

auto collect_and_validate(std::string_view input, const std::vector<MockFile>& imports)
    -> std::pair<SemaTestContext, usize> {
    auto [ctx, idx] = collect(input, imports);
    if (ctx.root_mod->has_sema_diagnostics()) {
        check_errors<sema::Diagnostic>(ctx.root_mod->get_sema_diagnostics());
        CHECK(ctx.root_mod->has_sema_diagnostics());
    }
    return {std::move(ctx), idx};
}

auto common_decl(std::string_view name, std::string_view assign) -> ast::DeclStatement {
    return ast::DeclStatement{
        syntax::Token{syntax::keywords::CONSTANT},
        make_ident(name),
        mem::make_box<ast::TypeExpression>(syntax::Token{syntax::operators::WALRUS}, opt::none),
        make_ident<true>(assign),
        ast::DeclModifiers::CONSTANT,
    };
}

} // namespace porpoise::tests::helpers

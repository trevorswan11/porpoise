#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/symbol_collector.hpp"
#include "sema/type_resolver.hpp"

namespace porpoise::sema {

auto Analyzer::analyze(const std::filesystem::path& entry_path) -> Result<Unit, Diagnostic> {
    auto module = TRY(modules_.try_get_file_module(entry_path));
    collect_symbols(*module);
    return Unit{};
}

auto Analyzer::collect_symbols(mod::Module& module) -> void {
    Diagnostics diagnostics{module.path};
    SymbolCollector::collect_symbols(module, {modules_, registry_, pool_, diagnostics});
}

} // namespace porpoise::sema

#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/symbol_collector.hpp"
#include "sema/type_resolver.hpp"

namespace porpoise::sema {

auto Analyzer::analyze(const std::filesystem::path& entry_path) -> Result<Unit, Diagnostic> {
    auto module_result = modules_.try_get_file_module(entry_path);
    if (!module_result) {
        return make_sema_err(std::move(module_result.error().get_message()),
                             Error::MODULE_LOAD_ERROR);
    }

    auto module = *module_result;
    if (module->has_parser_diagnostics()) {
        module->print_diagnostics(error_stream_);
        return Unit{};
    }

    collect_symbols(*module);
    resolve_types(*module);
    return Unit{};
}

auto Analyzer::collect_symbols(mod::Module& module) -> void {
    Diagnostics diagnostics{in_terminal_};
    SymbolCollector::collect_symbols(module,
                                     {modules_, registry_, pool_, diagnostics, error_stream_});
}

auto Analyzer::resolve_types(mod::Module& module) -> void {
    Diagnostics diagnostics{in_terminal_};
    TypeResolver::resolve_types(module, {modules_, registry_, pool_, diagnostics, error_stream_});
}

} // namespace porpoise::sema

#include "sema/analyzer.hh"

#include "sema/error.hh"
#include "sema/passes/symbol_collector.hh"
#include "sema/passes/type_resolver.hh"

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

    // Perform a final diagnostic flush if poisoned
    if (module->is_poisoned()) { module->print_diagnostics(error_stream_); }
    return Unit{};
}

auto Analyzer::collect_symbols(mod::Module& module) -> mod::ModuleState {
    return SymbolCollector::collect_symbols(module, ctx_);
}

auto Analyzer::resolve_types(mod::Module& module) -> mod::ModuleState {
    Context ctx = ctx_;
    return TypeResolver::resolve_types(module, ctx);
}

} // namespace porpoise::sema

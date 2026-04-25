#include "ast/ast.hpp"

#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/symbol.hpp"
#include "sema/symbol_collector.hpp"
#include "sema/type_resolver.hpp"

namespace porpoise::sema {

auto Analyzer::analyze(const std::filesystem::path& entry_path) -> void {}

auto Analyzer::collect_symbols(mod::Module& module) -> void {
    if (module.state >= mod::ModuleState::SYMBOLS_COLLECTED) { return; }
    if (!module.root_table_idx) {
        module.root_table_idx.emplace(registry_.create());

        Diagnostics     diagnostics;
        SymbolCollector collector{module.root_table_idx, registry_, pool_, diagnostics};
        for (const auto& node : module.tree) {
            node->accept(collector);
            collector.pass_first();
        }

        if (!diagnostics.empty()) {
            module.state = mod::ModuleState::ERRORED;
            module.diagnostics.emplace<Diagnostics>(std::move(diagnostics));
            return;
        }
        module.state = mod::ModuleState::SYMBOLS_COLLECTED;
    }
}

} // namespace porpoise::sema

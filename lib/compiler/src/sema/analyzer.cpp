#include "ast/ast.hpp"

#include "sema/analyzer.hpp"
#include "sema/symbol.hpp"
#include "sema/symbol_collector.hpp"
#include "sema/type_resolver.hpp"

namespace porpoise::sema {

auto Analyzer::collect_symbols() -> usize {
    root_idx_ = registry_.create();
    SymbolCollector collector{root_idx_, registry_, pool_, diagnostics_};
    for (const auto& node : tree_) {
        node->accept(collector);
        collector.pass_first();
    }
    return root_idx_;
}

auto Analyzer::resolve_symbols() -> void {
    auto&            root_table = registry_.get(root_idx_);
    SymbolTableStack stack;
    for (auto& [_, symbol] : root_table) {
        const auto result = resolve_symbol(symbol, stack);
        if (!result) { diagnostics_.emplace_back(result.error()); }
    }
}

auto Analyzer::resolve_symbol(Symbol& symbol, SymbolTableStack& stack) -> Result<Unit, Diagnostic> {
    switch (symbol.get_status()) {
    case ResolveStatus::RESOLVED: return {};
    case ResolveStatus::RESOLVING:
        return make_sema_err(
            fmt::format("Circular dependency detected for symbol '{}'", symbol.get_name()),
            Error::CIRCULAR_DEPENDENCY,
            symbol.get_node_token());
    case ResolveStatus::UNRESOLVED: break;
    }

    symbol.set_status(ResolveStatus::RESOLVING);
    TypeResolver resolver{*this, stack};
    symbol.match([&](const auto& node) { node->accept(resolver); });

    symbol.set_status(ResolveStatus::RESOLVED);
    return {};
}

} // namespace porpoise::sema

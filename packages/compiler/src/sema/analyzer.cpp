#include "sema/analyzer.hpp"
#include "sema/collector.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

auto Analyzer::collect_symbols(ast::ASTView ast) -> usize {
    const auto      idx = registry_.create();
    SymbolCollector collector{idx, registry_, pool_, diagnostics_};

    for (const auto& node : ast) {
        node->accept(collector);
        collector.pass_first();
    }
    return idx;
}

} // namespace porpoise::sema

#include <ranges>

#include "sema/collector.hpp"

#include "ast/ast.hpp"

namespace porpoise::sema {

auto SymbolCollector::collect(const ast::AST& ast) -> std::pair<SymbolTable, Diagnostics> {
    SymbolTable     table;
    Diagnostics     diagnostics;
    SymbolCollector collector{table, diagnostics};

    for (const auto& [node, i] : std::views::zip(ast, std::views::iota(0))) {
        if (i != 0 && node->is<ast::ModuleStatement>()) {
            diagnostics.emplace_back("Module statement must be first statement of file",
                                     SemaError::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                     node->get_token());
            continue;
        }
        node->accept(collector);
    }
    return {std::move(table), std::move(diagnostics)};
}

} // namespace porpoise::sema

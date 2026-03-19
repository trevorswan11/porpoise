#include "sema/collector.hpp"

#include "ast/ast.hpp"

namespace porpoise::sema {

auto SymbolCollector::collect(const ast::AST& ast) -> SymbolTable {
    SymbolTable     table;
    SymbolCollector collector{table};
    for (const auto& node : ast) { node->accept(collector); }
    return table;
}

} // namespace porpoise::sema

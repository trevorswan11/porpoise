#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/symbol.hpp"

namespace porpoise::sema {

// A very shallow AST-consumer for pass 1.
// - Collects top-level declarations only
// - Performs 0 type-checking
// - Does not verify undeclared identifier use
class SymbolCollector : public ast::Visitor {
  public:
    explicit SymbolCollector(SymbolTable& table) noexcept : table_{table} {}

    [[nodiscard]] static auto collect(const ast::AST& ast) -> SymbolTable;

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    SymbolTable table_;
};

} // namespace porpoise::sema

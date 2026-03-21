#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/error.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// A very shallow AST walker for pass 1.
// - Collects top-level declarations only
// - Performs 0 type-checking
// - Does not verify undeclared identifier use
class SymbolCollector : public ast::Visitor {
  public:
    [[nodiscard]] static auto collect(ast::ASTView ast) -> std::pair<SymbolTable, Diagnostics>;

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    explicit SymbolCollector(SymbolTable& table, Diagnostics& diagnostics) noexcept
        : table_{table}, diagnostics_{diagnostics} {}

  private:
    SymbolTable& table_;
    Diagnostics& diagnostics_;
    bool         first_node_{true};
};

} // namespace porpoise::sema

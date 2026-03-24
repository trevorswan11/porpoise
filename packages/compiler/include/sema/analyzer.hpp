#pragma once

#include "ast/node.hpp"

#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// The manager for all steps of semantic analysis.
class Analyzer {
  public:
    [[nodiscard]] auto collect_symbols(ast::ASTView ast) -> usize;

    [[nodiscard]] auto get_table(usize idx) -> SymbolTable& { return registry_.get(idx); }
    [[nodiscard]] auto get_table_opt(usize idx) noexcept -> Optional<SymbolTable&> {
        return registry_.get_opt(idx);
    }

    MAKE_GETTER(registry, const SymbolTableRegistry&)
    MAKE_GETTER(pool, const TypePool&)
    MAKE_GETTER(diagnostics, const Diagnostics&)

    auto has_diagnostics() const noexcept -> bool { return !diagnostics_.empty(); }

  private:
    SymbolTableRegistry registry_;
    TypePool            pool_;
    Diagnostics         diagnostics_;
};

} // namespace porpoise::sema

#pragma once

#include "ast/node.hpp"

#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// The manager for all steps of semantic analysis.
class Analyzer {
  public:
    explicit Analyzer(ast::AST tree) noexcept : tree_{std::move(tree)} {}
    ~Analyzer() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Analyzer)

    [[nodiscard]] auto collect_symbols() -> usize;
    auto               resolve_symbols() -> void;

    template <typename Self> [[nodiscard]] auto get_table(this Self&& self, usize idx) -> auto& {
        return self.registry_.get(idx);
    }

    template <typename Self>
    [[nodiscard]] auto get_table_opt(this Self&& self, usize idx) noexcept {
        return self.registry_.get_opt(idx);
    }

    MAKE_DEDUCING_GETTER(registry, SymbolTableRegistry&)
    MAKE_DEDUCING_GETTER(pool, TypePool&)
    MAKE_GETTER(diagnostics, const Diagnostics&)

    auto has_diagnostics() const noexcept -> bool { return !diagnostics_.empty(); }
    template <typename... Args> auto push_diagnostic(Args&&... args) -> void {
        diagnostics_.emplace_back(std::forward<Args>(args)...);
    }

  private:
    [[nodiscard]] auto resolve_symbol(Symbol& symbol, SymbolTableStack& stack)
        -> Expected<std::monostate, Diagnostic>;

  private:
    ast::AST            tree_;
    SymbolTableRegistry registry_;
    TypePool            pool_;
    Diagnostics         diagnostics_;
    usize               root_idx_;
};

} // namespace porpoise::sema

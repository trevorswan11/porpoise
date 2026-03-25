#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/error.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

class TypePool;

// A very shallow AST walker for pass 1.
// - Collects top-level declarations only
// - Performs 0 type-checking
// - Does not verify undeclared identifier use
class SymbolCollector : public ast::Visitor {
  public:
    SymbolCollector(usize                table_idx,
                    SymbolTableRegistry& registry,
                    TypePool&            pool,
                    Diagnostics&         diagnostics) noexcept
        : table_idx_{table_idx}, registry_{registry}, pool_{pool}, diagnostics_{diagnostics} {}

    MAKE_AST_VISITOR_OVERRIDES()

    auto pass_first() noexcept -> void { first_node_ = false; }

  private:
    usize                                 table_idx_;
    [[maybe_unused]] SymbolTableRegistry& registry_;
    [[maybe_unused]] TypePool&            pool_;
    Diagnostics&                          diagnostics_;
    bool                                  first_node_{true};
};

} // namespace porpoise::sema

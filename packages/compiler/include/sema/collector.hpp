#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/error.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

class TypePool;
class Type;

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
    // Returns false if the passed result was an error type
    template <typename T = std::monostate>
    auto try_result(Expected<T, SemaDiagnostic>&& result) -> bool {
        if (!result) {
            diagnostics_.emplace_back(result.error());
            return false;
        }
        return true;
    }

    auto visit(const ast::Enumeration&) -> void;
    auto visit(const ast::UnionField&) -> void;

  private:
    usize                                 table_idx_;
    [[maybe_unused]] SymbolTableRegistry& registry_;
    [[maybe_unused]] TypePool&            pool_;
    Diagnostics&                          diagnostics_;

    SymbolTableStack table_stack_;
    bool             first_node_{true};
    Optional<Type&>  last_type_;
};

} // namespace porpoise::sema

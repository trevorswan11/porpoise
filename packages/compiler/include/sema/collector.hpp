#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/error.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"
#include "sema/type.hpp"

namespace porpoise::sema {

// A very shallow AST walker for pass 1.
// - Collects top-level declarations only
// - Performs 0 type-checking
// - Does not verify undeclared identifier use
class SymbolCollector : public ast::Visitor {
  public:
    // Manages a new scopes symbol table
    class Scope {
      public:
        Scope(SymbolTableStack& s, usize new_idx, usize& old_idx) noexcept
            : guard_{s, new_idx}, idx_ref_{old_idx}, old_idx_{old_idx} {
            old_idx = new_idx;
        }
        ~Scope() { idx_ref_ = old_idx_; }

      private:
        SymbolTableStack::Guard guard_;
        usize&                  idx_ref_;
        usize                   old_idx_;
    };

  public:
    SymbolCollector(usize                table_idx,
                    SymbolTableRegistry& registry,
                    TypePool&            pool,
                    Diagnostics&         diagnostics) noexcept
        : table_idx_{table_idx}, registry_{registry}, pool_{pool}, diagnostics_{diagnostics} {
        table_stack_.push(table_idx);
    }

    MAKE_AST_VISITOR_OVERRIDES()

    auto pass_first() noexcept -> void { first_node_ = false; }

  private:
    template <typename T, typename VisitorFn>
    auto visit_scope(const T& expr, TypeKind kind, VisitorFn fn) -> void {
        const auto  new_idx = registry_.create();
        const Scope s{table_stack_, new_idx, table_idx_};
        for (const auto& field : expr) { fn(field); }
        last_type_.emplace(pool_.get_or_emplace(types::Key(kind, false, new_idx)));
    }

    // Returns false if the passed result was an error type
    template <typename T = std::monostate>
    auto try_result(Expected<T, Diagnostic>&& result) -> bool {
        if (!result) {
            diagnostics_.emplace_back(result.error());
            return false;
        }
        return true;
    }

    template <typename SymbolicVariant>
    auto try_declare(std::string_view name, SymbolicVariant* node) -> bool {
        return try_result(registry_.is_shadowing(table_stack_, name, node)) &&
               try_result(registry_.insert_into(table_idx_, name, node));
    }

    auto visit(const ast::Enumeration&) -> void;
    auto visit(const ast::UnionField&) -> void;

  private:
    usize                table_idx_;
    SymbolTableStack     table_stack_;
    SymbolTableRegistry& registry_;
    TypePool&            pool_;
    Diagnostics&         diagnostics_;

    bool            first_node_{true};
    Optional<Type&> last_type_;
};

} // namespace porpoise::sema

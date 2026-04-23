#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/error.hpp"
#include "sema/pool.hpp"
#include "sema/symbol.hpp"
#include "sema/type.hpp"

#include "counter.hpp"

namespace porpoise::sema {

// An AST walker that performs 0 type checking
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
    template <typename... IterPairs>
    [[nodiscard]] auto visit_scopes(TypeKind kind, IterPairs&&... pairs) -> usize {
        const auto  new_idx = registry_.create();
        const Scope s{table_stack_, new_idx, table_idx_};
        (..., [&] {
            for (const auto& item : pairs.iterable) { pairs.visitor(item); }
        }());
        last_type_.emplace(pool_[{kind, false, new_idx}]);
        return new_idx;
    }

    // Returns false if the passed result was an error type
    template <typename T = Unit> auto try_result(Result<T, Diagnostic>&& result) -> bool {
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

    auto loop_guard() noexcept -> std::pair<DefaultCounter::Guard, DefaultCounter::Guard> {
        return {in_loop_scope_.guard(), in_expr_scope_.guard()};
    }

    auto fn_guard() noexcept -> std::pair<DefaultCounter::Guard, DefaultCounter::Guard> {
        return {in_function_scope_.guard(), in_expr_scope_.guard()};
    }

  private:
    usize                table_idx_;
    SymbolTableStack     table_stack_;
    SymbolTableRegistry& registry_;
    TypePool&            pool_;
    Diagnostics&         diagnostics_;

    bool               first_node_{true};
    opt::Option<Type&> last_type_;
    DefaultCounter     in_function_scope_;
    DefaultCounter     in_loop_scope_;
    DefaultCounter     in_expr_scope_;
};

} // namespace porpoise::sema

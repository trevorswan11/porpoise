#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "module/module.hpp"

#include "sema/context.hpp"
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
    SymbolCollector(mod::Module& collecting, const Context& ctx) noexcept
        : collecting_{collecting}, table_idx_{collecting.root_table_idx}, ctx_{ctx} {
        table_stack_.push(table_idx_);
    }

    static auto collect_symbols(mod::Module& module, const Context& ctx) -> mod::ModuleState;

    MAKE_AST_VISITOR_OVERRIDES()

  private:
    template <typename... IterPairs>
    [[nodiscard]] auto visit_scopes(TypeKind kind, IterPairs&&... pairs) -> usize {
        const auto  new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};
        (..., [&] {
            for (const auto& item : pairs.iterable) { pairs.visitor(item); }
        }());
        last_type_.emplace(ctx_.pool[{kind, false, new_idx}]);
        return new_idx;
    }

    template <typename SymbolicVariant>
    auto try_declare(std::string_view name, SymbolicVariant node) -> bool {
        return ctx_.try_result(ctx_.registry.is_shadowing(table_stack_, name, node)) &&
               ctx_.try_result(ctx_.registry.insert_into(table_idx_, name, node));
    }

    auto fn_guard() noexcept -> std::pair<DefaultCounter::Guard, DefaultCounter::Guard> {
        return {in_function_scope_.guard(), in_expr_scope_.guard()};
    }

    auto loop_guard() noexcept -> std::pair<DefaultCounter::Guard, DefaultCounter::Guard> {
        return {in_loop_scope_.guard(), in_expr_scope_.guard()};
    }

    auto label_guard() noexcept -> std::pair<DefaultCounter::Guard, DefaultCounter::Guard> {
        return {in_label_scope_.guard(), in_expr_scope_.guard()};
    }

  private:
    mod::Module&       collecting_;
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context            ctx_;
    opt::Option<Type&> last_type_;

    DefaultCounter in_expr_scope_;
    DefaultCounter in_function_scope_;
    DefaultCounter in_loop_scope_;
    DefaultCounter in_label_scope_;
};

} // namespace porpoise::sema

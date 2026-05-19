#pragma once

#include <string_view>
#include <utility>

#include "ast/statement.hh"
#include "ast/traits.hh"
#include "ast/visitor.hh"
#include "module/error.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "counter.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"

namespace porpoise::sema {

// An AST walker that performs 0 type checking
class SymbolCollector {
  public:
    static auto collect_symbols(mod::Module& module, Context& ctx) -> mod::ModuleState;

    template <traits::IndexableID ID> auto collect(ID id) -> void {
        std::visit([&](const auto& data) { visit(id, data); }, collecting_.ast[id]);
    }

  private:
    using Scope = SymbolTableStack::Scope;

  private:
    AST_VISITOR_DEF_GEN()

    [[nodiscard]] auto collect_import_payload(const ast::ImportStatement& import_stmt)
        -> std::pair<std::string_view, Result<mem::NonNull<mod::Module>, mod::Diagnostic>>;

    template <typename... IterPairs>
    [[nodiscard]] auto visit_scopes(TypeKind kind, IterPairs&&... pairs) -> usize {
        const auto  new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};
        (..., [&pairs] {
            for (const auto& item : pairs.iterable) { pairs.visitor(item); }
        }());
        last_type_.emplace(ctx_.pool[{kind, types::mut::CONSTANT, new_idx}]);
        return new_idx;
    }

    template <typename SymbolicVariant, typename... Args>
    auto try_declare(std::string_view name, Args&&... args) -> bool {
        const SymbolicVariant node{std::forward<Args>(args)...};
        return ctx_.try_result(ctx_.registry.is_shadowing(table_stack_, collecting_, name, node)) &&
               ctx_.try_result(ctx_.registry.insert_into(table_idx_, collecting_, name, node));
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

    SymbolCollector(mod::Module& collecting, Context& ctx)
        : collecting_{collecting}, table_idx_{*collecting.root_table_idx}, ctx_{ctx} {
        table_stack_.push(table_idx_);
    }

  private:
    mod::Module&       collecting_;
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context&           ctx_;
    opt::Option<Type&> last_type_;

    DefaultCounter in_expr_scope_;
    DefaultCounter in_function_scope_;
    DefaultCounter in_loop_scope_;
    DefaultCounter in_label_scope_;
};

} // namespace porpoise::sema

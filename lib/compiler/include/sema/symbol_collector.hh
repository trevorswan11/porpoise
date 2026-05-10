#pragma once

#include "ast/ast.hh"

#include "module/module.hh"

#include "sema/context.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "counter.hh"

namespace porpoise::sema {

// An AST walker that performs 0 type checking
class SymbolCollector {
  public:
    static auto collect_symbols(mod::Module& module, Context& ctx) -> mod::ModuleState;

    auto collect(const ast::NodeID& id) -> void {
        ASSERT(id.is_valid(), "Attempt to collect invalid handle");
        std::visit([&](const auto& data) { visit(id, data); }, collecting_.forest[id]);
    }

    template <ast::NodeKind... Kinds> auto collect(const ast::Handle<Kinds...>& id) -> void {
        collect(*id);
    }

    auto collect(const ast::ExplicitTypeID& id) -> void {
        ASSERT(id.is_valid(), "Attempt to collect invalid handle");
        std::visit([&](const auto& data) { visit(id, data); }, collecting_.forest[id]);
    }

  private:
    AST_VISITOR_DEF_GEN()

    template <typename... IterPairs>
    [[nodiscard]] auto visit_scopes(TypeKind kind, IterPairs&&... pairs) -> usize {
        const auto  new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};
        (..., [&] {
            for (const auto& item : pairs.iterable) { pairs.visitor(item); }
        }());
        last_type_.emplace(ctx_.pool[{kind, types::Key::Mutability::IMMUTABLE, new_idx}]);
        return new_idx;
    }

    template <typename SymbolicVariant>
    auto try_declare(std::string_view name, SymbolicVariant node) -> bool {
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

  private:
    using Scope = SymbolTableStack::Scope;

  private:
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

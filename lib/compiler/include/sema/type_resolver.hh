#pragma once

#include "ast/ast.hh"

#include "sema/context.hh"
#include "sema/symbol.hh"

namespace porpoise::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    auto resolve(const ast::NodeID& id) -> void {
        ASSERT(id.is_valid(), "Attempt to resolve invalid handle");
        std::visit([&](const auto& data) { visit(id, data); }, collecting_.ast[id]);
    }

    template <ast::NodeKind... Kinds> auto resolve(const ast::Handle<Kinds...>& id) -> void {
        resolve(*id);
    }

    auto resolve(const ast::ExplicitTypeID& id) -> void {
        ASSERT(id.is_valid(), "Attempt to resolve invalid handle");
        std::visit([&](const auto& data) { visit(id, data); }, collecting_.ast[id]);
    }

  private:
    using Scope = SymbolTableStack::Stack;

  private:
    AST_VISITOR_DEF_GEN()

    TypeResolver(mod::Module& collecting, Context& ctx)
        : collecting_{collecting}, table_idx_{*collecting.root_table_idx}, ctx_{ctx} {
        ASSERT(ctx.prelude_index, "TypeResolver must be used post prelude-injection");
        table_stack_.push(*ctx_.prelude_index);
        table_stack_.push(table_idx_);
    }

  private:
    mod::Module&       collecting_;
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context&           ctx_;
    opt::Option<Type&> last_type_;
};

} // namespace porpoise::sema

#pragma once

#include "ast/nodes.hh"

#include "sema/context.hh"
#include "sema/symbol.hh"

namespace porpoise::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    AST_VISITOR_DEF_GEN()

  private:
    using Scope = SymbolTableStack::Stack;

  private:
    TypeResolver(mod::Module& collecting, Context& ctx)
        : table_idx_{*collecting.root_table_idx}, ctx_{ctx} {
        ASSERT(ctx.prelude_index, "TypeResolver must be used post prelude-injection");
        table_stack_.push(*ctx_.prelude_index);
        table_stack_.push(table_idx_);
    }

  private:
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context&           ctx_;
    opt::Option<Type&> last_type_;
};

} // namespace porpoise::sema

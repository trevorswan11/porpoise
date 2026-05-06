#pragma once

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "sema/context.hpp"
#include "sema/symbol.hpp"

namespace porpoise::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver : public ast::Visitor {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    MAKE_AST_VISITOR_OVERRIDES()

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

#pragma once

#include <span>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/traits.hh"
#include "ast/visitor.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::sema {

// Resolves all types and symbol uses without type checking
class TypeResolver {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    template <ast::traits::IndexableID ID> auto resolve(ID id) -> void {
        std::visit([&](const auto& data) { visit(id, data); }, resolving_.ast[id]);
    }

  private:
    using Scope = SymbolTableStack::Scope;

    // A flag for helper functions to indicate if their resolution was poisoned
    enum class ResolveResult : u8 {
        OK,
        POISONED,
    };

  private:
    // Poisons the node id and sets the `last_type_` pointer to be poisoned
    auto poison_node(ast::NodeID id) -> void;

    AST_VISITOR_DEF_GEN()

    [[nodiscard]] auto resolve_builtin_call(ast::NodeID                   id,
                                            const ast::CallExpression&    call,
                                            const types::BuiltinFunction& builtin)
        -> Result<Unit, Diagnostic>;

    auto resolve_call_args(std::span<const ast::CallExpression::Argument> args) -> ResolveResult;
    [[nodiscard]] auto get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
        -> mem::NonNull<Type>;
    [[nodiscard]] auto get_call_arg_location(const ast::CallExpression::Argument& arg)
        -> SourceLocation;

    [[nodiscard]] auto resolve_members(std::span<const ast::MemberHandle> members)
        -> opt::Option<std::span<mem::NonNull<Type>>>;

    TypeResolver(mod::Module& resolving, Context& ctx)
        : resolving_{resolving}, table_idx_{*resolving.root_table_idx}, ctx_{ctx} {
        ASSERT(ctx.prelude_index, "TypeResolver must be used post prelude-injection");
        table_stack_.push(*ctx_.prelude_index);
        table_stack_.push(table_idx_);
    }

  private:
    mod::Module&       resolving_;
    usize              table_idx_;
    SymbolTableStack   table_stack_;
    Context&           ctx_;
    opt::Option<Type&> last_type_;
};

} // namespace porpoise::sema

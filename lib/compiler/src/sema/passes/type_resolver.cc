#include "sema/passes/type_resolver.hh"

#include <span>
#include <utility>
#include <variant>

#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/type.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "memory.hh"
#include "variant.hh"

namespace porpoise::sema {

auto TypeResolver::resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState {
    // Poisoned collection should flush the diagnostics
    if (module.state == mod::ModuleState::POISONED_SYMBOL_COLLECTION) {
        module.print_diagnostics(ctx.error_stream);
    }

    if (module.is_resolvable()) {
        ctx.inject_prelude();

        TypeResolver resolver{module, ctx};
        for (const auto& node : module.ast) { resolver.resolve(node); }

        if (!ctx.diagnostics.empty() || module.is_poisoned()) {
            return module.error_out(std::move(ctx.diagnostics),
                                    mod::ModuleState::POISONED_TYPE_RESOLVED);
        }
        module.state = mod::ModuleState::TYPE_RESOLVED;
    }
    return module.state;
}

auto TypeResolver::visit(ast::NodeID id, const ast::ArrayExpression& array) -> void {
    for (const auto& item : array.items) { resolve(item); }
    resolve(array.item_explicit_type);
    mem::NonNull<Type> item_type = last_type_.take();
    collecting_.set_sema_type(array.item_explicit_type, *item_type);

    const auto items_size      = array.items.size();
    const auto null_terminated = array.null_terminated;
    last_type_.emplace(ctx_.pool[{
        TypeKind::ARRAY, types::mut::CONSTANT, null_terminated, items_size, *item_type}]);
    if (!last_type_->has_resolved()) {
        last_type_->resolve<types::Array>(*item_type, items_size, null_terminated);
    }
    collecting_.set_sema_type(id, *last_type_);
}

auto TypeResolver::resolve_call_args(std::span<const ast::CallExpression::Argument> args) -> void {
    for (const auto& arg : args) {
        std::visit(
            [this](const auto& handle) {
                resolve(handle);
                collecting_.set_sema_type(handle, *last_type_.take());
            },
            arg);
    }
}

auto TypeResolver::get_resolved_arg_type(const ast::CallExpression::Argument& arg)
    -> mem::NonNull<Type> {
    return std::visit(
        [this](const auto& handle) -> auto& {
            auto& type = collecting_.get_sema_type(handle);
            ASSERT(type.has_resolved(), "Call argument was not already resolved");
            return type;
        },
        arg);
}

// This is called after the call expression's arguments have all been resolved
auto TypeResolver::resolve_builtin_call(ast::NodeID                   id,
                                        const ast::CallExpression&    call,
                                        const types::BuiltinFunction& builtin) -> void {
    ASSERT(call.function.is<ast::IdentifierExpression>(), "Builtin function must be a raw ident");
    const auto& params = builtin.params;
    if (call.arguments.size() != params.size()) {
        ctx_.diagnostics.emplace_back(fmt::format("Builtin expects {} arguments but found {}",
                                                  params.size(),
                                                  call.arguments.size()),
                                      Error::ARITY_MISMATCH,
                                      collecting_.ast.location_of(id));
        collecting_.set_sema_type(id, ctx_.get_poison());
        return last_type_.emplace(ctx_.get_poison());
    }

    // There must be an actual builtin present with a token id
    const auto builtin_id = call.function->get_token_type();
    ASSERT(syntax::get_builtin_opt(builtin_id), "Cannot resolve non-builtin function");

    using syntax::TokenType;
    mem::NonNull<Type> result_type = ctx_.get_poison();

    // Indexing can be done freely as arity is already validated
    switch (builtin_id) {
    case TokenType::BUILTIN_ALIGN_CAST:
    case TokenType::BUILTIN_PTR_CAST:
    case TokenType::BUILTIN_BIT_CAST:
    case TokenType::BUILTIN_AS:         {
        // These builtins take in a resulting type to cast to
        result_type = get_resolved_arg_type(call.arguments[0]);
        break;
    }
    case TokenType::BUILTIN_CONST_CAST: {
        auto& expr_type = *get_resolved_arg_type(call.arguments[0]);
        result_type = ctx_.pool.strip_const(expr_type);
        break;
    }
    case TokenType::BUILTIN_VOLATILE_CAST: {
        auto& expr_type = *get_resolved_arg_type(call.arguments[0]);
        result_type = ctx_.pool.strip_volatile(expr_type);
        break;
    }
    case TokenType::BUILTIN_INT_FROM_PTR:   break;
    case TokenType::BUILTIN_PTR_FROM_INT:   break;
    case TokenType::BUILTIN_PTR_FROM_ARRAY: break;
    case TokenType::BUILTIN_SLICE_FROM_PTR: break;
    case TokenType::BUILTIN_ALIGN_OF:       break;
    case TokenType::BUILTIN_SIZE_OF:        break;
    case TokenType::BUILTIN_TYPE_OF:        break;
    case TokenType::BUILTIN_TAG_NAME:       break;
    case TokenType::BUILTIN_MEMCPY:
    case TokenType::BUILTIN_MEMSET:
    case TokenType::BUILTIN_MEMMOVE:        {
        ASSERT(builtin.return_type.get_kind() == TypeKind::VOID);
        result_type = builtin.return_type;
        break;
    }
    case TokenType::BUILTIN_MUL_ADD:   break;
    case TokenType::BUILTIN_CLZ:       break;
    case TokenType::BUILTIN_CTZ:       break;
    case TokenType::BUILTIN_DIV_MOD:   break;
    case TokenType::BUILTIN_POP_COUNT: break;
    case TokenType::BUILTIN_SQRT:      break;
    case TokenType::BUILTIN_SIN:       break;
    case TokenType::BUILTIN_COS:       break;
    case TokenType::BUILTIN_TAN:       break;
    case TokenType::BUILTIN_EXP:       break;
    case TokenType::BUILTIN_EXP2:      break;
    case TokenType::BUILTIN_LOG:       break;
    case TokenType::BUILTIN_LOG2:      break;
    case TokenType::BUILTIN_LOG10:     break;
    case TokenType::BUILTIN_ABS:       break;
    case TokenType::BUILTIN_FLOOR:     break;
    case TokenType::BUILTIN_CEIL:      break;
    case TokenType::BUILTIN_PANIC:     break;
    default:                           ASSERT(false, "Unimplemented builtin type resolution"); break;
    }

    collecting_.set_sema_type(id, *result_type);
    last_type_.emplace(*result_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::CallExpression& call) -> void {
    // The call can only yield a non-poison type if the function is valid
    resolve(call.function);
    mem::NonNull<Type> callee_type = last_type_.take();
    if (!callee_type->has_resolved() || callee_type->is_poison()) {
        collecting_.set_sema_type(id, ctx_.get_poison());
        resolve_call_args(call.arguments);
        return last_type_.emplace(ctx_.get_poison());
    }
    collecting_.set_sema_type(call.function, *callee_type);

    // Verify that the type in the function is callable and store the return type
    if (auto function_type = callee_type->as_opt<types::Function>()) {
        // Check the arity of the function against params before resetting last type
        resolve_call_args(call.arguments);
        const auto& params = function_type->params;
        if (call.arguments.size() != params.size()) {
            ctx_.diagnostics.emplace_back(fmt::format("Expected {} arguments but found {}",
                                                      params.size(),
                                                      call.arguments.size()),
                                          Error::ARITY_MISMATCH,
                                          collecting_.ast.location_of(id));
        }

        // TODO: Check argument-parameter type compat

        collecting_.set_sema_type(id, function_type->return_type);
        last_type_.emplace(function_type->return_type);
    } else if (auto builtin_type = callee_type->as_opt<types::BuiltinFunction>()) {
        resolve_call_args(call.arguments);
        resolve_builtin_call(id, call, *builtin_type);
    } else {
        ctx_.diagnostics.emplace_back("Expression is not callable",
                                      Error::NON_CALLABLE_EXPRESSION,
                                      collecting_.ast.location_of(id));
        collecting_.set_sema_type(id, ctx_.get_poison());
        resolve_call_args(call.arguments);
        return last_type_.emplace(ctx_.get_poison());
    }
}

auto TypeResolver::visit(ast::NodeID, const ast::DoWhileLoopExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::ForLoopExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::FunctionExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID id, const ast::IdentifierExpression& ident) -> void {
    const auto name   = ident.name;
    auto       symbol = ctx_.registry.lookup(table_stack_, name);

    // Check for an undeclared identifier and poison the ident
    if (!symbol) {
        ctx_.poison_node(collecting_,
                         id,
                         fmt::format("Use of undeclared identifier '{}'", name),
                         Error::UNDECLARED_IDENTIFIER,
                         collecting_.ast.location_of(id));
        return last_type_.emplace(ctx_.get_poison());
    }

    // Identifiers can be anything that they're symbol is allowed to be
    collecting_.set_sema_type(id, symbol->get_type());
    last_type_.emplace(symbol->get_type());
}

auto TypeResolver::visit(ast::NodeID, const ast::IfExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::IndexExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::InfiniteLoopExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::AssignmentExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::BinaryExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::DotExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::RangeExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::InitializerExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::LabelExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::MatchExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::ReferenceExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::DereferenceExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::UnaryExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::ImplicitAccessExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::StringExpression&) -> void {}

#define MAKE_PRIMITIVE_RESOLVER(NodeType, kind)                                         \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType&) -> void {            \
        last_type_.emplace(ctx_.pool[{TypeKind::kind, types::mut::CONSTANT}]);          \
        if (!last_type_->has_resolved()) { last_type_->resolve<types::BuiltinType>(); } \
        collecting_.set_sema_type(id, *last_type_);                                     \
    }

MAKE_PRIMITIVE_RESOLVER(I32Expression, I32)
MAKE_PRIMITIVE_RESOLVER(I64Expression, I64)
MAKE_PRIMITIVE_RESOLVER(ISizeExpression, ISIZE)
MAKE_PRIMITIVE_RESOLVER(U32Expression, U32)
MAKE_PRIMITIVE_RESOLVER(U64Expression, U64)
MAKE_PRIMITIVE_RESOLVER(USizeExpression, USIZE)
MAKE_PRIMITIVE_RESOLVER(U8Expression, U8)
MAKE_PRIMITIVE_RESOLVER(BoolExpression, BOOL)
MAKE_PRIMITIVE_RESOLVER(VoidExpression, VOID)
MAKE_PRIMITIVE_RESOLVER(UndefinedExpression, UNDEFINED)
MAKE_PRIMITIVE_RESOLVER(F32Expression, F32)
MAKE_PRIMITIVE_RESOLVER(F64Expression, F64)

auto TypeResolver::visit(ast::NodeID, const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::StructExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::FunctionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitTypeID&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::StructExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::UnionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::TypeExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::UnionExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::WhileLoopExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::BlockStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::BreakStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::ContinueStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::DeclStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::DeferStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::DiscardStatement& discard) -> void {
    resolve(discard.discarded);
}

auto TypeResolver::visit(ast::NodeID, const ast::ExpressionStatement& expr) -> void {
    resolve(expr.expression);
}

auto TypeResolver::visit(ast::NodeID, const ast::ImportStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::ReturnStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::TestStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const ast::UsingStatement&) -> void {}

auto TypeResolver::visit(ast::NodeID, const Unit&) -> void {}

} // namespace porpoise::sema

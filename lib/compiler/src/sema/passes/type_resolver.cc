#include "sema/passes/type_resolver.hh"

#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <variant>

#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/type.hh"
#include "module/module.hh"
#include "sema/context.hh"
#include "sema/error.hh"
#include "sema/symbol.hh"
#include "sema/type.hh"
#include "syntax/builtins.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "memory.hh"
#include "option.hh"
#include "result.hh"
#include "types.hh"
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

// Performs the resolution and poison check & bubble & return operation
#define TRY_RESOLVE(resolvable_expr)                                                       \
    do {                                                                                   \
        resolve(resolvable_expr);                                                          \
        if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); } \
    } while (0)

auto TypeResolver::visit(ast::NodeID id, const ast::ArrayExpression& array) -> void {
    for (const auto& item : array.items) { resolve(item); }
    resolve(array.item_explicit_type);
    auto& item_type = *last_type_.take();
    resolving_.set_sema_type(array.item_explicit_type, item_type);

    const auto items_size      = array.items.size();
    const auto null_terminated = array.null_terminated;
    last_type_.emplace(
        ctx_.pool[{TypeKind::ARRAY, types::mut::CONSTANT, null_terminated, items_size, item_type}]);
    if (!last_type_->has_resolved()) {
        last_type_->resolve<types::Array>(item_type, items_size, null_terminated);
    }
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::resolve_call_args(std::span<const ast::CallExpression::Argument> args)
    -> ResolveResult {
    bool any_poison = false;
    for (const auto& arg : args) {
        any_poison |= std::visit(
            [this](const auto& handle) {
                resolve(handle);
                const auto is_poison = last_type_->is_poison();
                resolving_.set_sema_type(handle, *last_type_.take());
                return is_poison;
            },
            arg);
    }
    return any_poison ? ResolveResult::POISONED : ResolveResult::OK;
}

auto TypeResolver::get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
    -> mem::NonNull<Type> {
    return std::visit(
        [this](const auto& handle) -> auto& {
            auto& type = resolving_.get_sema_type(handle);
            ASSERT(type.has_resolved(), "Call argument was not already resolved");
            return type;
        },
        arg);
}

auto TypeResolver::get_call_arg_location(const ast::CallExpression::Argument& arg)
    -> SourceLocation {
    return std::visit([this](auto id) { return resolving_.ast.location_of(id); }, arg);
}

// This is called after the call expression's arguments have all been resolved
auto TypeResolver::resolve_builtin_call(ast::NodeID                   id,
                                        const ast::CallExpression&    call,
                                        const types::BuiltinFunction& builtin)
    -> Result<Unit, Diagnostic> {
    ASSERT(call.function.is<ast::IdentifierExpression>(), "Builtin function must be a raw ident");
    const auto& params = builtin.params;
    if (call.arguments.size() != params.size()) {
        return make_sema_err(fmt::format("Builtin expects {} arguments but found {}",
                                         params.size(),
                                         call.arguments.size()),
                             Error::ARITY_MISMATCH,
                             resolving_.ast.location_of(id));
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
        result_type = get_resolved_call_arg_type(call.arguments[0]);
        break;
    }
    case TokenType::BUILTIN_CONST_CAST:
    case TokenType::BUILTIN_VOLATILE_CAST: {
        auto& expr_type = *get_resolved_call_arg_type(call.arguments[0]);
        if (expr_type.get_kind() != TypeKind::POINTER &&
            expr_type.get_kind() != TypeKind::REFERENCE) {
            return make_sema_err(fmt::format("Expected pointer or reference type, found {}",
                                             type_kind_display_name(expr_type.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }

        // Pointer/reference is checked since it is an invariant of the cast
        result_type = builtin_id == TokenType::BUILTIN_CONST_CAST
                          ? ctx_.pool.strip_const(expr_type)
                          : ctx_.pool.strip_volatile(expr_type);
        break;
    }
    case TokenType::BUILTIN_INT_FROM_PTR:
    case TokenType::BUILTIN_ALIGN_OF:
    case TokenType::BUILTIN_SIZE_OF:
    case TokenType::BUILTIN_CLZ:
    case TokenType::BUILTIN_CTZ:
    case TokenType::BUILTIN_POP_COUNT:    {
        ASSERT(builtin.return_type.get_kind() == TypeKind::USIZE);
        result_type = builtin.return_type;
        break;
    }
    case TokenType::BUILTIN_TYPE_OF: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::TYPE);
        result_type = builtin.return_type;
        break;
    }
    case TokenType::BUILTIN_PTR_FROM_ARRAY: {
        auto& array_type = *get_resolved_call_arg_type(call.arguments[0]);
        if (const auto& array_data = array_type.as_opt<types::Array>()) {
            auto array_key = array_type.get_key();
            array_key.set_kind(TypeKind::POINTER);

            // The new type uses the slightly modified key with the same underlying type
            auto& new_type = ctx_.pool[array_key];
            if (!new_type.has_resolved()) {
                new_type.resolve<types::Pointer>(array_data->underlying);
            }
            result_type = new_type;
        } else {
            return make_sema_err(fmt::format("Expected an array type, found {}",
                                             type_kind_display_name(array_type.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }
        break;
    }
    case TokenType::BUILTIN_PTR_FROM_INT: {
        auto& requested_output = *get_resolved_call_arg_type(call.arguments[0]);
        if (requested_output.get_kind() != TypeKind::POINTER) {
            return make_sema_err(fmt::format("Expected a pointer type, found {}",
                                             type_kind_display_name(requested_output.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }
        result_type = requested_output;
        break;
    }
    case TokenType::BUILTIN_SLICE_FROM_PTR: {
        auto& ptr_type = *get_resolved_call_arg_type(call.arguments[0]);
        if (const auto& ptr_data = ptr_type.as_opt<types::Pointer>()) {
            auto slice_key = ptr_type.get_key();
            slice_key.set_kind(TypeKind::SLICE);

            // The resulting slice isn't null terminated since the pointer gives no guarantee
            auto& new_type = ctx_.pool[slice_key];
            if (!new_type.has_resolved()) {
                new_type.resolve<types::Slice>(ptr_data->underlying, false);
            }
            result_type = new_type;
        } else {
            return make_sema_err(fmt::format("Expected a pointer type, found {}",
                                             type_kind_display_name(ptr_type.get_kind())),
                                 Error::TYPE_MISMATCH,
                                 get_call_arg_location(call.arguments[0]));
        }
        break;
    }
    case TokenType::BUILTIN_TAG_NAME: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::SLICE);
        result_type = builtin.return_type;
        break;
    }
    case TokenType::BUILTIN_MEMCPY:
    case TokenType::BUILTIN_MEMSET:
    case TokenType::BUILTIN_MEMMOVE: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::VOID);
        result_type = builtin.return_type;
        break;
    }
    // Many of these return @typeOf(expression) which is trivial
    case TokenType::BUILTIN_MUL_ADD:
    case TokenType::BUILTIN_SQRT:
    case TokenType::BUILTIN_SIN:
    case TokenType::BUILTIN_COS:
    case TokenType::BUILTIN_TAN:
    case TokenType::BUILTIN_EXP:
    case TokenType::BUILTIN_EXP2:
    case TokenType::BUILTIN_LOG:
    case TokenType::BUILTIN_LOG2:
    case TokenType::BUILTIN_LOG10:
    case TokenType::BUILTIN_ABS:
    case TokenType::BUILTIN_FLOOR:
    case TokenType::BUILTIN_CEIL:    {
        result_type = get_resolved_call_arg_type(call.arguments[0]);
        break;
    }
    // @divMod(T: type, lhs: T, rhs: T): struct { var quotient: T; var modulo: T; }
    case TokenType::BUILTIN_DIV_MOD: {
        auto& member_type = *get_resolved_call_arg_type(call.arguments[0]);
        auto  members     = ctx_.pool.get_many(2, member_type.get_key());

        // Structs might not be unique with the exact same calls
        result_type =
            ctx_.pool[{TypeKind::STRUCT, types::mut::CONSTANT, members.data(), members.size()}];
        if (!result_type->has_resolved()) { result_type->resolve<types::Struct>(members); }
        break;
    }
    case TokenType::BUILTIN_PANIC: {
        ASSERT(builtin.return_type.get_kind() == TypeKind::NORETURN);
        result_type = builtin.return_type;
        break;
    }
    default: ASSERT(false, "Unimplemented builtin type resolution"); break;
    }

    resolving_.set_sema_type(id, *result_type);
    last_type_.emplace(*result_type);
    return Unit{};
}

auto TypeResolver::visit(ast::NodeID id, const ast::CallExpression& call) -> void {
    // The call can only yield a non-poison type if the function is valid
    resolve(call.function);
    auto& callee_type = *last_type_.take();
    if (!callee_type.has_resolved() || callee_type.is_poison()) {
        resolve_call_args(call.arguments);
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
    resolving_.set_sema_type(call.function, callee_type);

    // There's no need to check any further if the arguments are poisoned
    if (resolve_call_args(call.arguments) == ResolveResult::POISONED) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }

    // Verify that the type in the function is callable and store the return type
    if (auto function_type = callee_type.as_opt<types::Function>()) {
        // Check the arity of the function against params before resetting last type
        const auto& params = function_type->params;
        if (call.arguments.size() != params.size()) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format(
                    "Expected {} arguments but found {}", params.size(), call.arguments.size()),
                Error::ARITY_MISMATCH,
                resolving_.ast.location_of(id)));
        }

        // Only arity is checked since the type checker will handle the rest
        resolving_.set_sema_type(id, function_type->return_type);
        last_type_.emplace(function_type->return_type);
    } else if (auto builtin_type = callee_type.as_opt<types::BuiltinFunction>()) {
        // Poison the call if there's an error early
        auto result = resolve_builtin_call(id, call, *builtin_type);
        if (!result) {
            return last_type_.emplace(ctx_.poison_node(resolving_, id, std::move(result.error())));
        }
    } else {
        return last_type_.emplace(ctx_.poison_node(resolving_,
                                                   id,
                                                   "Expression is not callable",
                                                   Error::NON_CALLABLE_EXPRESSION,
                                                   resolving_.ast.location_of(id)));
    }
}

auto TypeResolver::visit(ast::NodeID id, const ast::DoWhileLoopExpression& do_while) -> void {
    // The loop itself holds the block index, not the block
    auto& loop_type = resolving_.get_sema_type(id);
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};
        const auto& block = resolving_.ast.get_as<ast::BlockStatement>(do_while.block);
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    TRY_RESOLVE(do_while.condition);
    last_type_.emplace(loop_type);
}

auto TypeResolver::resolve_members(std::span<const ast::MemberHandle> members)
    -> opt::Option<std::span<mem::NonNull<Type>>> {
    // The member types need to be preallocated and can be left freely on error
    auto member_types = ctx_.pool.get_many_unsafe(members.size());

    // Only poison the enum once all members are collected
    bool member_poisoned = false;
    for (usize i = 0; const auto& member : members) {
        // The resolved statement type is in last_type_ unlike in the symbol collector
        resolve(*member);
        if (last_type_->is_poison()) { member_poisoned = true; }
        member_types[i++] = last_type_.take();
    }
    if (member_poisoned) { return opt::none; }
    return member_types;
}

auto TypeResolver::visit(ast::NodeID id, const ast::ForLoopExpression& for_expr) -> void {
    for (const auto& iterable : for_expr.iterables) { TRY_RESOLVE(iterable); }
    ASSERT(for_expr.iterables.size() == for_expr.captures.size());

    // The loop itself holds the block index which houses captures, not the block
    auto& loop_type = resolving_.get_sema_type(id);
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};

        // The captures must be paired with the iterables inner types (shallow type check)
        for (const auto& [capture, iterable] :
             std::views::zip(for_expr.captures, for_expr.iterables)) {
            // Assign types unconditionally since ignoring discards saves no space
            if (const auto& range = resolving_.ast.get_as_opt<ast::RangeExpression>(iterable)) {
                ASSERT(resolving_.has_sema_type(range->lhs), "Iterable was not resolved");
                auto& inner = resolving_.get_sema_type(range->lhs);
                resolving_.set_sema_type(capture.payload, inner);
                break;
            }

            ASSERT(resolving_.has_sema_type(iterable), "Iterable was not resolved");
            auto& iterable_type = resolving_.get_sema_type(iterable);
            if (const auto array = iterable_type.as_opt<types::Array>()) {
                resolving_.set_sema_type(capture.payload, array->underlying);
            } else if (const auto pointer = iterable_type.as_opt<types::Pointer>()) {
                resolving_.set_sema_type(capture.payload, pointer->underlying);
            } else {
                return last_type_.emplace(ctx_.poison_node(
                    resolving_,
                    id,
                    fmt::format("Iterables may only be ranges, arrays, or pointers. Found {}",
                                type_kind_display_name(iterable_type.get_kind())),
                    Error::TYPE_MISMATCH,
                    resolving_.ast.location_of(iterable)));
            }

            if (capture.payload.is<ast::IdentifierExpression>()) {
                resolve_symbol_info(capture.payload, SymbolKind::VALUE);
            }
        }
        const auto& block = resolving_.ast.get_as<ast::BlockStatement>(for_expr.block);
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    if (for_expr.non_break) { TRY_RESOLVE(*for_expr.non_break); }
    last_type_.emplace(loop_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::FunctionExpression& fn) -> void {
    // The entire function lives inside of its preallocated scope
    auto&       fn_type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, fn_type.get_symbol_table_idx(), table_idx_};

    if (fn.self) {
        TRY_RESOLVE(fn.self->ident);
        resolve_symbol_info(fn.self->ident, SymbolKind::VALUE);
    }

    for (const auto& param : fn.parameters) {
        TRY_RESOLVE(param.ident);
        resolve_symbol_info(param.ident, SymbolKind::VALUE);
        TRY_RESOLVE(param.explicit_type);
    }

    TRY_RESOLVE(fn.explicit_return_type);
    const auto& block = resolving_.ast.get_as<ast::BlockStatement>(fn.body);
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(fn_type);
}

auto TypeResolver::resolve_symbol_info(ast::IdentifierHandle handle, opt::Option<SymbolKind> kind)
    -> void {
    const auto& ident  = resolving_.ast.get_as<ast::IdentifierExpression>(handle);
    auto&       symbol = ctx_.registry.get_from(table_idx_, ident.name);
    if (kind) { symbol.set_kind(*kind); }
    symbol.set_status(SymbolStatus::RESOLVED);
}

auto TypeResolver::visit(ast::NodeID id, const ast::IdentifierExpression& ident) -> void {
    const auto name       = ident.name;
    auto       symbol_opt = ctx_.registry.lookup(table_stack_, name);

    // Check for an undeclared identifier and poison the ident
    if (!symbol_opt) {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Use of undeclared identifier '{}'", name),
                             Error::UNDECLARED_IDENTIFIER,
                             resolving_.ast.location_of(id)));
    }
    auto& symbol = *symbol_opt;

    switch (symbol.get_status()) {
    case SymbolStatus::RESOLVED:
        ASSERT(resolving_.has_sema_type(id), "Resolved ident has no type");
        break;
    case SymbolStatus::RESOLVING:
        symbol.set_status(SymbolStatus::RESOLVED);
        symbol.set_kind(SymbolKind::POISONED);
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Cycle: '{}' is used during its own resolution", name),
                             Error::CYCLIC_DEPENDENCY,
                             resolving_.ast.location_of(id)));
    case SymbolStatus::UNRESOLVED: {
        symbol.set_status(SymbolStatus::RESOLVING);

        // All other symbol data kinds are independently resolved
        const auto node = symbol.as_opt<symbols::Node>();
        ASSERT(node, "Unresolved symbol is not AST-associated");
        resolve(*node);
        resolving_.set_sema_type(id, *last_type_.take());
        break;
    }
    default: std::unreachable();
    }

    if (symbol.get_kind() == SymbolKind::POISONED) {
        return last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
    last_type_.emplace(resolving_.get_sema_type(id));
}

auto TypeResolver::visit(ast::NodeID id, const ast::IfExpression& if_expr) -> void {
    TRY_RESOLVE(if_expr.condition);
    TRY_RESOLVE(if_expr.consequence);

    // There can only be a non-void type with an alternate but this is for pass 3
    auto& branch_type = *last_type_.take();
    if (if_expr.alternate) { TRY_RESOLVE(*if_expr.alternate); }
    resolving_.set_sema_type(id, branch_type);
    last_type_.emplace(branch_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::IndexExpression& index) -> void {
    TRY_RESOLVE(index.array);
    auto& array_type = *last_type_.take();

    if (const auto slice = array_type.as_opt<types::Slice>()) {
        last_type_.emplace(slice->underlying);
    } else if (const auto array = array_type.as_opt<types::Array>()) {
        last_type_.emplace(array->underlying);
    } else if (const auto pointer = array_type.as_opt<types::Pointer>()) {
        last_type_.emplace(pointer->underlying);
    } else {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Can only index slices, arrays, and pointers, found {}",
                                         type_kind_display_name(array_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }

    auto& single_item_type = *last_type_.take();
    TRY_RESOLVE(index.index);

    resolving_.set_sema_type(id, single_item_type);
    last_type_.emplace(single_item_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::InfiniteLoopExpression& loop) -> void {
    TRY_RESOLVE(loop.block);
    last_type_.emplace(resolving_.get_sema_type(id));
}

#define MAKE_INFIX_RESOLVER(NodeType)                                             \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType& node) -> void { \
        TRY_RESOLVE(node.lhs);                                                    \
        TRY_RESOLVE(node.rhs);                                                    \
        resolving_.set_sema_type(id, *last_type_);                                \
    }

MAKE_INFIX_RESOLVER(AssignmentExpression)
MAKE_INFIX_RESOLVER(BinaryExpression)
MAKE_INFIX_RESOLVER(DotExpression)
MAKE_INFIX_RESOLVER(RangeExpression)

auto TypeResolver::visit(ast::NodeID id, const ast::InitializerExpression& init) -> void {
    for (const auto& initializer : init.initializers) {
        TRY_RESOLVE(initializer.member);
        TRY_RESOLVE(initializer.value);
    }

    // Resolve the object second so it can be tied to the expression's type
    if (init.object_type) {
        TRY_RESOLVE(*init.object_type);
        resolving_.set_sema_type(id, resolving_.get_sema_type(*init.object_type));
        last_type_.emplace(resolving_.get_sema_type(id));
    } else {
        // Without an object type it must be inferred from the LHS
        auto& auto_type = ctx_.get_builtin_resolved_type(TypeKind::AUTO);
        resolving_.set_sema_type(id, auto_type);
        last_type_.emplace(auto_type);
    }
}

auto TypeResolver::visit(ast::NodeID id, const ast::LabelExpression& label) -> void {
    auto&       label_type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, label_type.get_symbol_table_idx(), table_idx_};

    // Resolve the body but cache the label's type so the result can bind to the label
    TRY_RESOLVE(*label.body);
    resolve_symbol_info(label.name, opt::none);
    last_type_.emplace(label_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::MatchExpression& match) -> void {
    TRY_RESOLVE(match.matcher);
    auto& matcher_type = *last_type_.take();

    // The expression must resolve to a single type on pass 3
    opt::Option<Type&> first_type;
    const auto         try_set_first_type = [&] {
        if (!first_type) {
            first_type = last_type_.take();
        } else {
            last_type_.reset();
        }
    };

    // Each arm was assigned a new scope index on the first pass
    for (const auto& arm : match.arms) {
        const auto& arm_type = resolving_.get_sema_type(arm.pattern);
        const Scope s{table_stack_, arm_type.get_symbol_table_idx(), table_idx_};

        if (arm.capture && arm.capture->is<ast::IdentifierExpression>()) {
            resolve_symbol_info(*arm.capture, opt::none);
            resolving_.set_sema_type(*arm.capture, matcher_type);
        }

        TRY_RESOLVE(arm.pattern);
        TRY_RESOLVE(arm.dispatch);

        try_set_first_type();
    }

    if (match.catch_all) {
        TRY_RESOLVE(*match.catch_all);
        try_set_first_type();
    }

    // In the rare case that a type could not be found we have to poison
    if (first_type) {
        resolving_.set_sema_type(id, *first_type);
        last_type_.emplace(*first_type);
    } else {
        last_type_.emplace(ctx_.poison_node(resolving_, id));
    }
}

#define MAKE_PREFIX_RESOLVER(NodeType)                                            \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType& node) -> void { \
        TRY_RESOLVE(node.rhs);                                                    \
        resolving_.set_sema_type(id, *last_type_);                                \
    }

namespace {

// Returns the mutability associated with the reference/address-of operator
[[nodiscard]] auto ref_adr_of_is_mutable(ast::NodeID id) noexcept -> types::MutabilityModifiers {
    using syntax::TokenType;
    switch (id.get_token_type()) {
    case TokenType::BW_AND:    return types::mut::CONSTANT;
    case TokenType::AND_MUT:   return types::mut::MUTABLE;
    case TokenType::CARET:     return types::mut::CONSTANT;
    case TokenType::CARET_MUT: return types::mut::MUTABLE;
    default:                   std::unreachable();
    }
}

} // namespace

auto TypeResolver::visit(ast::NodeID id, const ast::ReferenceExpression& ref) -> void {
    TRY_RESOLVE(ref.rhs);
    auto& rhs_type = *last_type_.take();

    auto& new_type = ctx_.pool[{TypeKind::REFERENCE, ref_adr_of_is_mutable(id), rhs_type}];
    new_type.resolve<types::Reference>(rhs_type);

    resolving_.set_sema_type(id, new_type);
    last_type_.emplace(new_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::AddressOfExpression& adr_of) -> void {
    TRY_RESOLVE(adr_of.rhs);
    auto& rhs_type = *last_type_.take();

    auto& new_type = ctx_.pool[{TypeKind::POINTER, ref_adr_of_is_mutable(id), rhs_type}];
    new_type.resolve<types::Pointer>(rhs_type);

    resolving_.set_sema_type(id, new_type);
    last_type_.emplace(new_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::DereferenceExpression& deref) -> void {
    TRY_RESOLVE(deref.rhs);
    auto& inner_type = *last_type_.take();

    // Check for a pointer and update to the underlying type to enforce dereference semantics
    if (const auto pointer = inner_type.as_opt<types::Pointer>()) {
        last_type_.emplace(pointer->underlying);
    } else {
        return last_type_.emplace(
            ctx_.poison_node(resolving_,
                             id,
                             fmt::format("Cannot dereference non-pointer expression, found {}",
                                         type_kind_display_name(inner_type.get_kind())),
                             Error::TYPE_MISMATCH,
                             resolving_.ast.location_of(id)));
    }
    resolving_.set_sema_type(id, *last_type_);
}

MAKE_PREFIX_RESOLVER(UnaryExpression)
MAKE_PREFIX_RESOLVER(ImplicitAccessExpression)

// String literals are just constant arrays of bytes
auto TypeResolver::visit(ast::NodeID id, const ast::StringExpression& string) -> void {
    const auto size = string.value.size();
    auto&      type = ctx_.pool[{TypeKind::ARRAY, types::mut::CONSTANT, false, TypeKind::U8, size}];

    // String literals with the same size will always have the same type
    type.resolve<types::Array>(ctx_.get_builtin_resolved_type(TypeKind::U8), size, false);
    resolving_.set_sema_type(id, type);
    last_type_.emplace(type);
}

#define MAKE_PRIMITIVE_RESOLVER(NodeType, kind)                                         \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType&) -> void {            \
        last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::kind));             \
        if (!last_type_->has_resolved()) { last_type_->resolve<types::BuiltinType>(); } \
        resolving_.set_sema_type(id, *last_type_);                                      \
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

auto TypeResolver::visit(ast::NodeID id, const ast::ScopeResolutionExpression& scope) -> void {
    // Resolving the right hand side recurses down to the identifier level
    TRY_RESOLVE(scope.outer);
    auto& outer_type = *last_type_.take();

    // User types use the dot operator so this is trivial to verify
    if (const auto module = outer_type.as_opt<types::Module>()) {
        // The module may not have been resolved yet due to order independence
        auto& inner_mod = module->imported;
        if (inner_mod.is_resolvable()) {
            Context new_ctx = ctx_;
            resolve_types(inner_mod, new_ctx);
            if (inner_mod.is_poisoned()) {
                return last_type_.emplace(ctx_.poison_node(resolving_, id));
            }
        }

        // Step into the module's scope for lookup
        const Scope s{table_stack_, *inner_mod.root_table_idx, table_idx_};
        const auto& inner_ident = resolving_.ast.get_as<ast::IdentifierExpression>(scope.inner);
        auto        symbol      = ctx_.registry.lookup(table_stack_, inner_ident.name);
        if (!symbol) {
            return last_type_.emplace(
                ctx_.poison_node(resolving_,
                                 id,
                                 fmt::format("Module has no member named '{}'", inner_ident.name),
                                 Error::UNDECLARED_IDENTIFIER,
                                 resolving_.ast.location_of(scope.inner)));
        } else if (symbol->get_status() != SymbolStatus::RESOLVED ||
                   !inner_mod.has_sema_type(scope.inner)) {
            return last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format("Module has member '{}' but it is unresolved", inner_ident.name),
                Error::UNDECLARED_IDENTIFIER,
                resolving_.ast.location_of(scope.inner)));
        }

        auto& ident_type = inner_mod.get_sema_type(scope.inner);
        resolving_.set_sema_type(scope.inner, ident_type);
        resolving_.set_sema_type(id, ident_type);
        return last_type_.emplace(ident_type);
    } else if (outer_type.as_opt<types::Struct>() || outer_type.as_opt<types::Enum>() ||
               outer_type.as_opt<types::Union>()) {
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            fmt::format(
                "Use the dot operator '.' to access {} members, found scope resolution '::'",
                type_kind_display_name(outer_type.get_kind())),
            Error::TYPE_MISMATCH,
            resolving_.ast.location_of(scope.outer)));
    } else {
        return last_type_.emplace(ctx_.poison_node(
            resolving_,
            id,
            fmt::format("Scope resolution operator '::' can only be applied to modules, found {}",
                        type_kind_display_name(outer_type.get_kind())),
            Error::TYPE_MISMATCH,
            resolving_.ast.location_of(scope.outer)));
    }
}

auto TypeResolver::visit(ast::NodeID id, const ast::WhileLoopExpression& while_loop) -> void {
    TRY_RESOLVE(while_loop.condition);
    if (while_loop.continuation) { TRY_RESOLVE(*while_loop.continuation); }
    // The loop itself holds the block index which houses captures, not the block
    auto& loop_type = resolving_.get_sema_type(id);
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};
        const auto& block = resolving_.ast.get_as<ast::BlockStatement>(while_loop.block);
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    if (while_loop.non_break) { TRY_RESOLVE(*while_loop.non_break); }
    last_type_.emplace(loop_type);
}

// DONT CALL ME FROM ANY LOOP/CONDITION/FN RESOLVER
auto TypeResolver::visit(ast::NodeID id, const ast::BlockStatement& block) -> void {
    auto&       block_type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, block_type.get_symbol_table_idx(), table_idx_};

    // Just an abridged loop handler
    for (const auto& stmt : block.statements) { TRY_RESOLVE(stmt); }
    last_type_.emplace(block_type);
}

auto TypeResolver::resolve_control_flow_label(ast::NodeID                        id,
                                              opt::Option<ast::IdentifierHandle> label,
                                              std::string_view stmt_name) -> bool {
    if (label) {
        const auto& ident  = resolving_.ast.get_as<ast::IdentifierExpression>(*label);
        auto&       symbol = ctx_.registry.get_from(table_idx_, ident.name);
        if (symbol.get_kind() != SymbolKind::LABEL) {
            last_type_.emplace(ctx_.poison_node(
                resolving_,
                id,
                fmt::format("Labeled {} statements must be used with a known label", stmt_name),
                Error::ILLEGAL_CONTROL_FLOW,
                resolving_.ast.location_of(*label)));
            return false;
        }
    }
    return true;
}

auto TypeResolver::visit(ast::NodeID id, const ast::BreakStatement& break_stmt) -> void {
    if (!resolve_control_flow_label(id, break_stmt.label, "break")) { return; }

    if (break_stmt.expression) {
        // Don't do anything with the label's type since it may need peer type resolution in pass 3
        TRY_RESOLVE(*break_stmt.expression);
        resolving_.set_sema_type(id, *last_type_.take());
    } else {
        // No payload is semantically equivalent to breaking with void
        resolving_.set_sema_type(id, ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ContinueStatement& continue_stmt) -> void {
    if (!resolve_control_flow_label(id, continue_stmt.label, "continue")) { return; }

    // Similar to breaks but there is no way to break with a value
    resolving_.set_sema_type(id, ctx_.get_builtin_resolved_type(TypeKind::VOID));
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DeclStatement& decl) -> void {
    const auto& ident      = resolving_.ast.get_as<ast::IdentifierExpression>(*decl.ident);
    auto        symbol_opt = ctx_.registry.lookup(table_stack_, ident.name);
    ASSERT(symbol_opt, "Somehow the declaration was lost in the symbol table");
    auto& symbol = *symbol_opt;

    // Breaking out early is possible due to out of order semantics
    if (symbol.get_status() == SymbolStatus::RESOLVED) {
        return last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
    }
    symbol.set_status(SymbolStatus::RESOLVING);

    // With an explicit type, the ident should always adopt that exact type
    if (decl.explicit_type) {
        TRY_RESOLVE(*decl.explicit_type);
        resolving_.set_sema_type(decl.ident, *last_type_.take());
    }

    if (decl.value) {
        TRY_RESOLVE(*decl.value);

        // The ident will not be typed if pass 1 didn't find a block type
        if (!resolving_.has_sema_type(decl.ident)) {
            ASSERT(!decl.explicit_type, "An invariant was violated in the declaration");
            resolving_.set_sema_type(decl.ident, *last_type_.take());
        } else {
            ASSERT(&resolving_.get_sema_type(*decl.value) == &*last_type_,
                   "Resolved type doesn't match collected");
        }
    }

    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

#define MAKE_PAYLOAD_STMT_RESOLVER(NodeType, member)                              \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType& node) -> void { \
        TRY_RESOLVE(node.member);                                                 \
        last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));       \
    }

MAKE_PAYLOAD_STMT_RESOLVER(DeferStatement, deferred)
MAKE_PAYLOAD_STMT_RESOLVER(DiscardStatement, discarded)
MAKE_PAYLOAD_STMT_RESOLVER(ExpressionStatement, expression)

auto TypeResolver::visit(ast::NodeID id, const ast::ImportStatement&) -> void {
    // Updating this type reflects on the symbol in the actual table as well
    auto& import_type = resolving_.get_sema_type(id);
    ASSERT(import_type.has_resolved(), "Import types should be resolved on pass 1");
    auto& module = import_type.as<types::Module>();

    // There's no need to poison the import since it would lose all of the module information
    Context new_ctx = ctx_;
    resolve_types(module.imported, new_ctx);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ReturnStatement& return_stmt) -> void {
    if (return_stmt.expression) { TRY_RESOLVE(*return_stmt.expression); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::TestStatement& test) -> void {
    auto&       test_type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, test_type.get_symbol_table_idx(), table_idx_};

    const auto& block = resolving_.ast.get_as<ast::BlockStatement>(test.block);
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::UsingStatement& using_stmt) -> void {
    TRY_RESOLVE(using_stmt.explicit_type);

    // Bind the resolved type to the symbol now that its been collected
    resolving_.set_sema_type(using_stmt.alias, *last_type_.take());
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, ast::ExplicitTypeID) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void {}

} // namespace porpoise::sema

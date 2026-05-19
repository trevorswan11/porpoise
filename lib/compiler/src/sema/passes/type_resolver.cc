#include "sema/passes/type_resolver.hh"

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

auto TypeResolver::poison_node(ast::NodeID id) -> void {
    ctx_.poison_node(resolving_, id);
    last_type_.emplace(ctx_.get_poison());
}

// Performs the resolution and poison check & bubble & return operation
#define TRY_RESOLVE(resolvable_expr)                             \
    do {                                                         \
        resolve(resolvable_expr);                                \
        if (last_type_->is_poison()) { return poison_node(id); } \
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
        return poison_node(id);
    }
    resolving_.set_sema_type(call.function, callee_type);

    // There's no need to check any further if the arguments are poisoned
    if (resolve_call_args(call.arguments) == ResolveResult::POISONED) { return poison_node(id); }

    // Verify that the type in the function is callable and store the return type
    if (auto function_type = callee_type.as_opt<types::Function>()) {
        // Check the arity of the function against params before resetting last type
        const auto& params = function_type->params;
        if (call.arguments.size() != params.size()) {
            ctx_.diagnostics.emplace_back(fmt::format("Expected {} arguments but found {}",
                                                      params.size(),
                                                      call.arguments.size()),
                                          Error::ARITY_MISMATCH,
                                          resolving_.ast.location_of(id));
            return poison_node(id);
        }

        // Only arity is checked since the type checker will handle the rest
        resolving_.set_sema_type(id, function_type->return_type);
        last_type_.emplace(function_type->return_type);
    } else if (auto builtin_type = callee_type.as_opt<types::BuiltinFunction>()) {
        // Poison the call if there's an error early
        auto result = resolve_builtin_call(id, call, *builtin_type);
        if (!result) {
            ctx_.diagnostics.emplace_back(std::move(result.error()));
            return poison_node(id);
        }
    } else {
        ctx_.diagnostics.emplace_back("Expression is not callable",
                                      Error::NON_CALLABLE_EXPRESSION,
                                      resolving_.ast.location_of(id));
        return poison_node(id);
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

auto TypeResolver::visit(ast::NodeID id, const ast::EnumExpression& enum_expr) -> void {
    if (enum_expr.underlying) { resolve(*enum_expr.underlying); }

    auto&       type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, type.get_symbol_table_idx(), table_idx_};

    for (const auto& [name, value] : enum_expr.enumerations) {
        // This is the first time the value is analyzed during sema
        TRY_RESOLVE(name);
        if (value) { TRY_RESOLVE(*value); }
    }

    auto member_types = resolve_members(enum_expr.members);
    if (!member_types) { return poison_node(id); }

    // The underlying type defaults to an i32 as it would in C or C++
    auto& underlying_type = enum_expr.underlying ? resolving_.get_sema_type(*enum_expr.underlying)
                                                 : ctx_.get_builtin_resolved_type(TypeKind::I32);
    type.resolve<types::Enum>(underlying_type, *member_types);
    last_type_.emplace(type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::ForLoopExpression& for_expr) -> void {
    for (const auto& iterable : for_expr.iterables) { TRY_RESOLVE(iterable); }

    // The loop itself holds the block index which houses captures, not the block
    auto& loop_type = resolving_.get_sema_type(id);
    {
        const Scope s{table_stack_, loop_type.get_symbol_table_idx(), table_idx_};
        for (const auto& capture : for_expr.captures) { TRY_RESOLVE(capture.payload); }
        const auto& block = resolving_.ast.get_as<ast::BlockStatement>(for_expr.block);
        for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    }

    if (for_expr.non_break) { TRY_RESOLVE(*for_expr.non_break); }
    last_type_.emplace(loop_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::FunctionExpression& fn) -> void {
    auto&       fn_type = resolving_.get_sema_type(id);
    const Scope s{table_stack_, fn_type.get_symbol_table_idx(), table_idx_};

    // The entire function lives inside of its preallocated scope
    if (fn.self) { TRY_RESOLVE(fn.self->ident); }
    for (const auto& param : fn.parameters) {
        TRY_RESOLVE(param.ident);
        TRY_RESOLVE(param.explicit_type);
    }
    TRY_RESOLVE(fn.explicit_return_type);

    const auto& block = resolving_.ast.get_as<ast::BlockStatement>(fn.body);
    for (const auto& stmt : block) { TRY_RESOLVE(stmt); }
    last_type_.emplace(fn_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::IdentifierExpression& ident) -> void {
    const auto name   = ident.name;
    auto       symbol = ctx_.registry.lookup(table_stack_, name);

    // Check for an undeclared identifier and poison the ident
    if (!symbol) {
        ctx_.poison_node(resolving_,
                         id,
                         fmt::format("Use of undeclared identifier '{}'", name),
                         Error::UNDECLARED_IDENTIFIER,
                         resolving_.ast.location_of(id));
        return last_type_.emplace(ctx_.get_poison());
    }
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
    } else {
        ctx_.diagnostics.emplace_back(fmt::format("Can only index slices and arrays, found {}",
                                                  type_kind_display_name(array_type.get_kind())),
                                      Error::TYPE_MISMATCH,
                                      resolving_.ast.location_of(id));
        return poison_node(id);
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
    last_type_.emplace(label_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::MatchExpression& match) -> void {
    TRY_RESOLVE(match.matcher);

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
        poison_node(id);
    }
}

#define MAKE_PREFIX_RESOLVER(NodeType)                                            \
    auto TypeResolver::visit(ast::NodeID id, const ast::NodeType& node) -> void { \
        TRY_RESOLVE(node.rhs);                                                    \
        resolving_.set_sema_type(id, *last_type_);                                \
    }

MAKE_PREFIX_RESOLVER(ReferenceExpression)

auto TypeResolver::visit(ast::NodeID id, const ast::DereferenceExpression& deref) -> void {
    TRY_RESOLVE(deref.rhs);
    auto& inner_type = *last_type_.take();

    // Check for a pointer and update to the underlying type to enforce dereference semantics
    if (const auto pointer = inner_type.as_opt<types::Pointer>()) {
        last_type_.emplace(pointer->underlying);
    } else {
        ctx_.diagnostics.emplace_back(
            fmt::format("Cannot dereference non-pointer expression, found {}",
                        type_kind_display_name(inner_type.get_kind())),
            Error::TYPE_MISMATCH,
            resolving_.ast.location_of(id));
        return poison_node(id);
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

auto TypeResolver::visit(ast::NodeID, const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(ast::NodeID id, const ast::StructExpression& struct_expr) -> void {
    auto member_types = resolve_members(struct_expr.members);
    if (!member_types) { return poison_node(id); }

    auto& type = resolving_.get_sema_type(id);
    type.resolve<types::Struct>(*member_types);
    last_type_.emplace(type);
}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitTypeID&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::StructExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::UnionExpression&) -> void {}

auto TypeResolver::visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void {}

auto TypeResolver::visit(ast::NodeID id, const ast::UnionExpression& union_expr) -> void {
    auto& type = resolving_.get_sema_type(id);

    auto field_types    = ctx_.pool.get_many_unsafe(union_expr.fields.size());
    bool field_poisoned = false;

    for (usize i = 0; const auto& field : union_expr.fields) {
        resolve(field.explicit_type);
        if (last_type_->is_poison()) { field_poisoned = true; }
        field_types[i++] = last_type_.take();
    }
    if (field_poisoned) { return poison_node(id); }

    auto member_types = resolve_members(union_expr.members);
    if (!member_types) { return poison_node(id); }

    type.resolve<types::Union>(field_types, *member_types);
    last_type_.emplace(type);
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

auto TypeResolver::visit(ast::NodeID id, const ast::BreakStatement& break_stmt) -> void {
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

auto TypeResolver::visit(ast::NodeID, const ast::ContinueStatement&) -> void {
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::NORETURN));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DeclStatement& decl) -> void {
    if (decl.type) { TRY_RESOLVE(*decl.type); }
    if (decl.value) {
        TRY_RESOLVE(*decl.value);

        // There might have already been a symbol present, which should be equivalent
        if (!resolving_.has_sema_type(decl.ident)) {
            resolving_.set_sema_type(decl.ident, *last_type_.take());
        } else {
            ASSERT(&resolving_.get_sema_type(decl.ident) == &*last_type_,
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

auto TypeResolver::visit(ast::NodeID, const Unit&) -> void {}

} // namespace porpoise::sema

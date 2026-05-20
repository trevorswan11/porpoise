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
#include "syntax/token_type.hh"

#include "assert.hh"
#include "diagnostic.hh"
#include "memory.hh"
#include "option.hh"
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
            ASSERT(resolving_.has_sema_type(iterable), "Iterable was not resolved");
            auto& iterable_type = resolving_.get_sema_type(iterable);
            if (const auto array = iterable_type.as_opt<types::Array>()) {
                resolving_.set_sema_type(capture.payload, array->underlying);
            } else if (const auto slice = iterable_type.as_opt<types::Slice>()) {
                resolving_.set_sema_type(capture.payload, slice->underlying);
            } else {
                return last_type_.emplace(
                    ctx_.poison_node(resolving_,
                                     id,
                                     fmt::format("Iterables may only be arrays or slices, found {}",
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
        // TODO: Set current user type being analyzed to set self and verify usage
        resolve_symbol_info(fn.self->ident, SymbolKind::VALUE);
    }

    auto param_types    = ctx_.pool.get_many_unsafe(fn.parameters.size());
    bool param_poisoned = false;

    // Every parameter contributes to the resolution but not the type key due to unique idx
    for (usize i = 0; const auto& param : fn.parameters) {
        resolve(param.explicit_type);
        if (last_type_->is_poison()) { param_poisoned = true; }

        auto& param_type = *last_type_.take();
        param_types[i++] = param_type;
        resolving_.set_sema_type(param.ident, param_type);
        resolve_symbol_info(param.ident, SymbolKind::VALUE);
    }
    if (param_poisoned) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

    TRY_RESOLVE(fn.explicit_return_type);
    auto& return_type = *last_type_.take();

    ASSERT(fn_type.has_resolved(), "Valued function must not be resolved");
    fn_type.resolve<types::Function>(param_types, return_type);

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

auto TypeResolver::visit(ast::NodeID id, const ast::AssignmentExpression& assign) -> void {
    TRY_RESOLVE(assign.lhs);
    TRY_RESOLVE(assign.rhs);

    // Only pass 3 can verify assignment allowance due to mutability semantics
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::BinaryExpression& binary) -> void {
    TRY_RESOLVE(binary.lhs);
    TRY_RESOLVE(binary.rhs);
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::DotExpression& dot) -> void { TODO(id, dot); }

auto TypeResolver::visit(ast::NodeID id, const ast::RangeExpression& range) -> void {
    TRY_RESOLVE(range.lhs);
    auto& lhs_type = *last_type_.take();
    TRY_RESOLVE(range.rhs);
    auto& rhs_type = *last_type_.take();

    // Due to deferred type checking just use one type
    auto& slice_type = ctx_.pool[{TypeKind::SLICE, types::mut::CONSTANT, lhs_type, rhs_type}];
    slice_type.resolve<types::Slice>(rhs_type, false);
    last_type_.emplace(slice_type);
    resolving_.set_sema_type(id, *last_type_);
}

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

namespace {

// Returns the mutability associated with the reference/address-of operator
[[nodiscard]] auto ref_addr_of_is_mutable(ast::NodeID id) noexcept -> types::MutabilityModifiers {
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

    auto& new_type = ctx_.pool[{TypeKind::REFERENCE, ref_addr_of_is_mutable(id), rhs_type}];
    new_type.resolve<types::Reference>(rhs_type);

    resolving_.set_sema_type(id, new_type);
    last_type_.emplace(new_type);
}

auto TypeResolver::visit(ast::NodeID id, const ast::AddressOfExpression& adr_of) -> void {
    TRY_RESOLVE(adr_of.rhs);
    auto& rhs_type = *last_type_.take();

    auto& new_type = ctx_.pool[{TypeKind::POINTER, ref_addr_of_is_mutable(id), rhs_type}];
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

auto TypeResolver::visit(ast::NodeID id, const ast::UnaryExpression& node) -> void {
    TRY_RESOLVE(node.rhs);
    resolving_.set_sema_type(id, *last_type_);
}

auto TypeResolver::visit(ast::NodeID id, const ast::ImplicitAccessExpression& node) -> void {
    TRY_RESOLVE(node.rhs);
    resolving_.set_sema_type(id, *last_type_);
}

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

auto TypeResolver::visit(ast::NodeID id, const ast::DeferStatement& defer) -> void {
    TRY_RESOLVE(defer.deferred);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::DiscardStatement& discard) -> void {
    TRY_RESOLVE(discard.discarded);
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ExpressionStatement& expr) -> void {
    TRY_RESOLVE(expr.expression);
    resolving_.set_sema_type(expr.expression, *last_type_.take());
    last_type_.emplace(ctx_.get_builtin_resolved_type(TypeKind::VOID));
}

auto TypeResolver::visit(ast::NodeID id, const ast::ImportStatement&) -> void {
    // Updating this type reflects on the symbol in the actual table as well
    auto& import_type = resolving_.get_sema_type(id);
    if (import_type.is_poison()) { return; }
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

namespace {

[[nodiscard]] auto mutability_from_type_modifier(ast::TypeModifier modifier) noexcept
    -> opt::Option<types::MutabilityModifiers> {
    using Modifier = ast::TypeModifier::Modifier;
    switch (modifier.get_raw()) {
    case Modifier::VALUE:        return opt::none;
    case Modifier::REF:          return types::mut::CONSTANT;
    case Modifier::MUT_REF:      return types::mut::MUTABLE;
    case Modifier::PTR:          return types::mut::CONSTANT;
    case Modifier::MUT_PTR:      return types::mut::MUTABLE;
    case Modifier::VOLATILE:     return types::mut::CONSTANT_VOLATILE;
    case Modifier::MUT_VOLATILE: return types::mut::VOLATILE;
    }
}

} // namespace

// Without a modifier or with poison the result should be the same as the node
auto TypeResolver::apply_explicit_modifiers(ast::ExplicitTypeID id, Type& inner_type) -> Type& {
    const auto modifier   = id.get_modifier();
    const auto mutability = mutability_from_type_modifier(modifier);
    if (!mutability || inner_type.is_poison()) { return inner_type; }

    auto new_key = inner_type.get_key();
    new_key.set_mut(*mutability);

    if (modifier.is_ptr()) {
        auto& new_ptr_type = ctx_.pool[new_key];
        if (!new_ptr_type.has_resolved()) { new_ptr_type.resolve<types::Pointer>(inner_type); }
        return new_ptr_type;
    } else if (modifier.is_ref()) {
        auto& new_ref_type = ctx_.pool[new_key];
        if (!new_ref_type.has_resolved()) { new_ref_type.resolve<types::Reference>(inner_type); }
        return new_ref_type;
    } else if (modifier.is_volatile()) {
        // Volatility doesn't impact the resolved type
        auto& new_vol_type = ctx_.pool[new_key];
        if (!new_vol_type.has_resolved()) { new_vol_type.resolve(inner_type.get_resolved()); }
        return new_vol_type;
    }
    std::unreachable();
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::IdentifierExpression& ident) -> void {
    resolve_ident(id, ident);
    auto& resolved = apply_explicit_modifiers(id, *last_type_.take());
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::ScopeResolutionExpression& scope)
    -> void {
    resolve_scope(id, scope);
    auto& resolved = apply_explicit_modifiers(id, *last_type_.take());
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::CallExpression& call) -> void {
    resolve_call(id, call);

    // The last type is guaranteed to be the return type or poison
    auto& resolved = apply_explicit_modifiers(id, *last_type_.take());
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::ExplicitFunctionType& fn) -> void {
    auto param_types    = ctx_.pool.get_many_unsafe(fn.parameter_types.size());
    bool param_poisoned = false;

    for (usize i = 0; const auto& param : fn.parameter_types) {
        resolve(param);
        if (last_type_->is_poison()) { param_poisoned = true; }
        param_types[i++] = last_type_.take();
    }
    if (param_poisoned) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

    TRY_RESOLVE(fn.explicit_return_type);
    auto& return_type = *last_type_.take();

    // Make a unique function type by imprinting every associated type
    types::Key fn_key{TypeKind::FUNCTION, types::mut::CONSTANT};
    for (const auto& param_type : param_types) { fn_key.imprint(*param_type); }
    fn_key.imprint(return_type);

    auto& resolved_fn = ctx_.pool[fn_key];
    if (!resolved_fn.has_resolved()) {
        resolved_fn.resolve<types::Function>(param_types, return_type);
    }

    auto& final_type = apply_explicit_modifiers(id, resolved_fn);
    resolving_.set_sema_type(id, final_type);
    last_type_.emplace(final_type);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, ast::ExplicitTypeID nested) -> void {
    TRY_RESOLVE(nested);
    auto& resolved = apply_explicit_modifiers(id, *last_type_.take());
    resolving_.set_sema_type(id, resolved);
    last_type_.emplace(resolved);
}

auto TypeResolver::visit(ast::ExplicitTypeID id, const ast::ExplicitArrayType& array) -> void {
    TRY_RESOLVE(array.inner_explicit_type);
    auto& item_type = *last_type_.take();

    const auto null_terminated = array.null_terminated;
    if (array.dimension) {
        TRY_RESOLVE(*array.dimension);
        const usize placeholder_size = 0;
        last_type_.emplace(ctx_.pool[{
            TypeKind::ARRAY, types::mut::CONSTANT, null_terminated, placeholder_size, item_type}]);
        if (!last_type_->has_resolved()) {
            last_type_->resolve<types::Array>(item_type, placeholder_size, null_terminated);
        }
    } else {
        last_type_.emplace(
            ctx_.pool[{TypeKind::SLICE, types::mut::CONSTANT, null_terminated, item_type}]);
        if (!last_type_->has_resolved()) {
            last_type_->resolve<types::Slice>(item_type, null_terminated);
        }
    }

    auto& final_type = apply_explicit_modifiers(id, *last_type_.take());
    resolving_.set_sema_type(id, final_type);
    last_type_.emplace(final_type);
}

} // namespace porpoise::sema

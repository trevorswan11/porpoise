#pragma once

#include <span>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/traits.hh"
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

// Resolves all types and symbol uses without type checking
class TypeResolver {
  public:
    static auto resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState;

    template <traits::IndexableID ID> auto resolve(ID id) -> void {
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
    auto visit(ast::NodeID, const ast::ArrayExpression&) -> void;

    // This is meant to be called after the arguments have all been resolved
    template <traits::IndexableID ID>
    [[nodiscard]] auto resolve_builtin_call(ID                            id,
                                            const ast::CallExpression&    call,
                                            const types::BuiltinFunction& builtin)
        -> Result<Unit, Diagnostic> {
        ASSERT(call.function.is<ast::IdentifierExpression>(),
               "Builtin function must be a raw ident");
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
        mem::NonNull<Type> return_type = ctx_.get_poison();

        // Indexing can be done freely as arity is already validated
        switch (builtin_id) {
        case TokenType::BUILTIN_ALIGN_CAST:
        case TokenType::BUILTIN_PTR_CAST:
        case TokenType::BUILTIN_BIT_CAST:
        case TokenType::BUILTIN_AS:         {
            // These builtins take in a resulting type to cast to
            return_type = get_resolved_call_arg_type(call.arguments[0]);
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
            return_type = builtin_id == TokenType::BUILTIN_CONST_CAST
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
            return_type = builtin.return_type;
            break;
        }
        case TokenType::BUILTIN_TYPE_OF: {
            ASSERT(builtin.return_type.get_kind() == TypeKind::TYPE);
            return_type = builtin.return_type;
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
                return_type = new_type;
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
                return make_sema_err(
                    fmt::format("Expected a pointer type, found {}",
                                type_kind_display_name(requested_output.get_kind())),
                    Error::TYPE_MISMATCH,
                    get_call_arg_location(call.arguments[0]));
            }
            return_type = requested_output;
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
                return_type = new_type;
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
            return_type = builtin.return_type;
            break;
        }
        case TokenType::BUILTIN_MEMCPY:
        case TokenType::BUILTIN_MEMSET:
        case TokenType::BUILTIN_MEMMOVE: {
            ASSERT(builtin.return_type.get_kind() == TypeKind::VOID);
            return_type = builtin.return_type;
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
            return_type = get_resolved_call_arg_type(call.arguments[0]);
            break;
        }
        // @divMod(T: type, lhs: T, rhs: T): struct { var quotient: T; var modulo: T; }
        case TokenType::BUILTIN_DIV_MOD: {
            auto& member_type = *get_resolved_call_arg_type(call.arguments[0]);
            auto  members     = ctx_.pool.get_many(2, member_type.get_key());

            // Structs might not be unique with the exact same calls
            return_type =
                ctx_.pool[{TypeKind::STRUCT, types::mut::CONSTANT, members.data(), members.size()}];
            if (!return_type->has_resolved()) { return_type->resolve<types::Struct>(members); }
            break;
        }
        case TokenType::BUILTIN_PANIC: {
            ASSERT(builtin.return_type.get_kind() == TypeKind::NORETURN);
            return_type = builtin.return_type;
            break;
        }
        default: ASSERT(false, "Unimplemented builtin type resolution"); break;
        }

        resolving_.set_sema_type(id, *return_type);
        last_type_.emplace(*return_type);
        return Unit{};
    }

    auto resolve_call_args(std::span<const ast::CallExpression::Argument> args) -> ResolveResult;
    [[nodiscard]] auto get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
        -> mem::NonNull<Type>;
    [[nodiscard]] auto get_call_arg_location(const ast::CallExpression::Argument& arg)
        -> SourceLocation;

    // Resolves the call to allow for simple forwarding through the type visitor
    template <traits::IndexableID ID>
    auto resolve_call(ID id, const ast::CallExpression& call) -> void {
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
                return last_type_.emplace(
                    ctx_.poison_node(resolving_, id, std::move(result.error())));
            }
        } else {
            return last_type_.emplace(ctx_.poison_node(resolving_,
                                                       id,
                                                       "Expression is not callable",
                                                       Error::NON_CALLABLE_EXPRESSION,
                                                       resolving_.ast.location_of(id)));
        }
    }

    auto visit(ast::NodeID id, const ast::CallExpression& call) -> void { resolve_call(id, call); }
    auto visit(ast::NodeID, const ast::DoWhileLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::ForLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::FunctionExpression&) -> void;

    // Resolves the identifier to allow for simple forwarding through the type visitor
    template <traits::IndexableID ID>
    auto resolve_ident(ID id, const ast::IdentifierExpression& ident) -> void {
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
            // TODO: Forward the resolved symbol type to the ident
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

    auto visit(ast::NodeID id, const ast::IdentifierExpression& ident) -> void {
        resolve_ident(id, ident);
    }

    auto visit(ast::NodeID, const ast::IfExpression&) -> void;
    auto visit(ast::NodeID, const ast::IndexExpression&) -> void;
    auto visit(ast::NodeID, const ast::InfiniteLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::AssignmentExpression&) -> void;
    auto visit(ast::NodeID, const ast::BinaryExpression&) -> void;
    auto visit(ast::NodeID, const ast::DotExpression&) -> void;
    auto visit(ast::NodeID, const ast::RangeExpression&) -> void;
    auto visit(ast::NodeID, const ast::InitializerExpression&) -> void;
    auto visit(ast::NodeID, const ast::LabelExpression&) -> void;
    auto visit(ast::NodeID, const ast::MatchExpression&) -> void;
    auto visit(ast::NodeID, const ast::ReferenceExpression&) -> void;
    auto visit(ast::NodeID, const ast::AddressOfExpression&) -> void;
    auto visit(ast::NodeID, const ast::DereferenceExpression&) -> void;
    auto visit(ast::NodeID, const ast::UnaryExpression&) -> void;
    auto visit(ast::NodeID, const ast::ImplicitAccessExpression&) -> void;
    auto visit(ast::NodeID, const ast::StringExpression&) -> void;
    auto visit(ast::NodeID, const ast::I32Expression&) -> void;
    auto visit(ast::NodeID, const ast::I64Expression&) -> void;
    auto visit(ast::NodeID, const ast::ISizeExpression&) -> void;
    auto visit(ast::NodeID, const ast::U32Expression&) -> void;
    auto visit(ast::NodeID, const ast::U64Expression&) -> void;
    auto visit(ast::NodeID, const ast::USizeExpression&) -> void;
    auto visit(ast::NodeID, const ast::U8Expression&) -> void;
    auto visit(ast::NodeID, const ast::F32Expression&) -> void;
    auto visit(ast::NodeID, const ast::F64Expression&) -> void;
    auto visit(ast::NodeID, const ast::BoolExpression&) -> void;
    auto visit(ast::NodeID, const ast::VoidExpression&) -> void;
    auto visit(ast::NodeID, const ast::UndefinedExpression&) -> void;

    // Resolves the call to allow for simple forwarding through the type visitor
    template <traits::IndexableID ID>
    auto resolve_scope(ID id, const ast::ScopeResolutionExpression& scope) -> void {
        // Resolving the right hand side recurses down to the identifier level
        resolve(scope.outer);
        if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); }
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
                return last_type_.emplace(ctx_.poison_node(
                    resolving_,
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
                fmt::format(
                    "Scope resolution operator '::' can only be applied to modules, found {}",
                    type_kind_display_name(outer_type.get_kind())),
                Error::TYPE_MISMATCH,
                resolving_.ast.location_of(scope.outer)));
        }
    }

    auto visit(ast::NodeID id, const ast::ScopeResolutionExpression& scope) -> void {
        resolve_scope(id, scope);
    }

    auto visit(ast::NodeID, const ast::WhileLoopExpression&) -> void;

    auto visit(ast::NodeID, const ast::BlockStatement&) -> void;

    // Returns `true` if the resolution was successful
    [[nodiscard]] auto resolve_control_flow_label(ast::NodeID                        id,
                                                  opt::Option<ast::IdentifierHandle> label,
                                                  std::string_view stmt_name) -> bool;

    auto visit(ast::NodeID, const ast::BreakStatement&) -> void;
    auto visit(ast::NodeID, const ast::ContinueStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeclStatement&) -> void;
    auto visit(ast::NodeID, const ast::DeferStatement&) -> void;
    auto visit(ast::NodeID, const ast::DiscardStatement&) -> void;
    auto visit(ast::NodeID, const ast::ExpressionStatement&) -> void;
    auto visit(ast::NodeID, const ast::ImportStatement&) -> void;
    auto visit(ast::NodeID, const ast::ReturnStatement&) -> void;
    auto visit(ast::NodeID, const ast::TestStatement&) -> void;
    auto visit(ast::NodeID, const ast::UsingStatement&) -> void;
    auto visit(ast::NodeID, const Unit&) noexcept -> void {}

    // Creates a potentially new type with the id-stored modifiers
    auto apply_explicit_modifiers(ast::ExplicitTypeID id, Type& inner_type) -> Type&;

    auto visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ScopeResolutionExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitTypeID) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void;

    [[nodiscard]] auto resolve_members(std::span<const ast::MemberHandle> members)
        -> opt::Option<std::span<mem::NonNull<Type>>>;

#define CHECK_STRUCTURED_EXPLICIT_TYPE_ID()                                   \
    if constexpr (std::is_same_v<ID, ast::ExplicitTypeID>) {                  \
        ASSERT(id.get_modifier().is_value(), "Structured type has modifier"); \
    }

    template <traits::IndexableID ID>
    auto visit(ID id, const ast::EnumExpression& enum_expr) -> void {
        CHECK_STRUCTURED_EXPLICIT_TYPE_ID();
        if (enum_expr.underlying) { resolve(*enum_expr.underlying); }

        auto&       type = resolving_.get_sema_type(id);
        const Scope s{table_stack_, type.get_symbol_table_idx(), table_idx_};

        for (const auto& [name, value] : enum_expr.enumerations) {
            // This is the first time the value is analyzed during sema
            if (value) {
                resolve(*value);
                if (last_type_->is_poison()) { return resolving_.set_sema_type(id, *last_type_); }
            }
            resolve_symbol_info(name, SymbolKind::VALUE);
        }

        auto member_types = resolve_members(enum_expr.members);
        if (!member_types) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        // The underlying type defaults to an i32 as it would in C or C++
        auto& underlying_type = enum_expr.underlying
                                    ? resolving_.get_sema_type(*enum_expr.underlying)
                                    : ctx_.get_builtin_resolved_type(TypeKind::I32);
        type.template resolve<types::Enum>(underlying_type, *member_types);
        last_type_.emplace(type);
    }

    template <traits::IndexableID ID>
    auto visit(ID id, const ast::StructExpression& struct_expr) -> void {
        CHECK_STRUCTURED_EXPLICIT_TYPE_ID();
        auto member_types = resolve_members(struct_expr.members);
        if (!member_types) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        auto& type = resolving_.get_sema_type(id);
        type.template resolve<types::Struct>(*member_types);
        last_type_.emplace(type);
    }

    template <traits::IndexableID ID>
    auto visit(ID id, const ast::UnionExpression& union_expr) -> void {
        CHECK_STRUCTURED_EXPLICIT_TYPE_ID();
        auto& type = resolving_.get_sema_type(id);

        auto field_types    = ctx_.pool.get_many_unsafe(union_expr.fields.size());
        bool field_poisoned = false;

        for (usize i = 0; const auto& field : union_expr.fields) {
            resolve(field.explicit_type);
            if (last_type_->is_poison()) { field_poisoned = true; }
            auto& field_type = *last_type_.take();
            field_types[i++] = field_type;

            // The field's name is able to be used as a value
            resolve_symbol_info(field.ident, SymbolKind::VALUE);
            resolving_.set_sema_type(field.ident, field_type);
        }
        if (field_poisoned) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        auto member_types = resolve_members(union_expr.members);
        if (!member_types) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        type.template resolve<types::Union>(field_types, *member_types);
        last_type_.emplace(type);
    }

#undef CHECK_STRUCTURED_EXPLICIT_TYPE_ID

    // Looks up the symbol by name and sets its status to resolved and alters its kind if requested
    auto resolve_symbol_info(ast::IdentifierHandle handle, opt::Option<SymbolKind> kind) -> void;

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

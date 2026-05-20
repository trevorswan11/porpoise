#pragma once

#include <span>
#include <string_view>

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

    [[nodiscard]] auto resolve_builtin_call(ast::NodeID                   id,
                                            const ast::CallExpression&    call,
                                            const types::BuiltinFunction& builtin)
        -> Result<Unit, Diagnostic>;

    auto resolve_call_args(std::span<const ast::CallExpression::Argument> args) -> ResolveResult;
    [[nodiscard]] auto get_resolved_call_arg_type(const ast::CallExpression::Argument& arg)
        -> mem::NonNull<Type>;
    [[nodiscard]] auto get_call_arg_location(const ast::CallExpression::Argument& arg)
        -> SourceLocation;

    auto visit(ast::NodeID, const ast::CallExpression&) -> void;
    auto visit(ast::NodeID, const ast::DoWhileLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::ForLoopExpression&) -> void;
    auto visit(ast::NodeID, const ast::FunctionExpression&) -> void;
    auto visit(ast::NodeID, const ast::IdentifierExpression&) -> void;
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
    auto visit(ast::NodeID, const ast::ScopeResolutionExpression&) -> void;
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

    auto visit(ast::ExplicitTypeID, const ast::IdentifierExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ScopeResolutionExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::CallExpression&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitFunctionType&) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitTypeID) -> void;
    auto visit(ast::ExplicitTypeID, const ast::ExplicitArrayType&) -> void;

    [[nodiscard]] auto resolve_members(std::span<const ast::MemberHandle> members)
        -> opt::Option<std::span<mem::NonNull<Type>>>;

    template <traits::IndexableID ID>
    auto visit(ID id, const ast::EnumExpression& enum_expr) -> void {
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
        auto member_types = resolve_members(struct_expr.members);
        if (!member_types) { return last_type_.emplace(ctx_.poison_node(resolving_, id)); }

        auto& type = resolving_.get_sema_type(id);
        type.template resolve<types::Struct>(*member_types);
        last_type_.emplace(type);
    }

    template <traits::IndexableID ID>
    auto visit(ID id, const ast::UnionExpression& union_expr) -> void {
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

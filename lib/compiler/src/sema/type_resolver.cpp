#include <cassert>

#include "sema/type.hpp"
#include "sema/type_resolver.hpp"

#include "ast/ast.hpp"

namespace porpoise::sema {

auto TypeResolver::resolve_types(mod::Module& module, const Context& ctx) -> mod::ModuleState {
    // Poisoned collection should flush the diagnostics
    if (module.state == mod::ModuleState::POISONED_SYMBOL_COLLECTION) {
        module.print_diagnostics(ctx.error_stream);
    }

    if (module.is_resolvable()) {
        TypeResolver resolver{ctx};
        for (const auto& node : module.tree) { node->accept(resolver); }

        if (!ctx.diagnostics.empty() || module.is_poisoned()) {
            return module.error_out(std::move(ctx.diagnostics),
                                    mod::ModuleState::POISONED_TYPE_RESOLVED);
        }
        module.state = mod::ModuleState::TYPE_RESOLVED;
    }
    return module.state;
}

auto TypeResolver::visit(const ast::ArrayExpression& array) -> void {
    visit_list(array.get_items());
    visit(array.get_item_type());
    array.get_item_type().set_sema_type(*last_type_.take());

    const auto size            = array.get_size();
    auto&      item_type       = array.get_item_type().get_sema_type();
    const auto null_terminated = array.is_null_terminated();
    last_type_.emplace(
        ctx_.pool[{TypeKind::ARRAY, false, 0, null_terminated, size, item_type.as_marker()}]);
    if (!last_type_->has_resolved()) {
        last_type_->resolve<types::Array>(item_type, size, null_terminated);
    }
    array.set_sema_type(*last_type_);
}

auto TypeResolver::visit(const ast::CallArgument&) -> void {}

auto TypeResolver::visit(const ast::CallExpression&) -> void {}

auto TypeResolver::visit(const ast::DoWhileLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::Enumeration&) -> void {}

auto TypeResolver::visit(const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(const ast::ForLoopCapture&) -> void {}

auto TypeResolver::visit(const ast::ForLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::SelfParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionExpression&) -> void {}

auto TypeResolver::visit(const ast::IdentifierExpression&) -> void {}

auto TypeResolver::visit(const ast::IfExpression&) -> void {}

auto TypeResolver::visit(const ast::IndexExpression&) -> void {}

auto TypeResolver::visit(const ast::InfiniteLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::AssignmentExpression&) -> void {}

auto TypeResolver::visit(const ast::BinaryExpression&) -> void {}

auto TypeResolver::visit(const ast::DotExpression&) -> void {}

auto TypeResolver::visit(const ast::RangeExpression&) -> void {}

auto TypeResolver::visit(const ast::Initializer&) -> void {}

auto TypeResolver::visit(const ast::InitializerExpression&) -> void {}

auto TypeResolver::visit(const ast::LabelExpression&) -> void {}

auto TypeResolver::visit(const ast::MatchArm&) -> void {}

auto TypeResolver::visit(const ast::MatchExpression&) -> void {}

auto TypeResolver::visit(const ast::ReferenceExpression&) -> void {}

auto TypeResolver::visit(const ast::DereferenceExpression&) -> void {}

auto TypeResolver::visit(const ast::UnaryExpression&) -> void {}

auto TypeResolver::visit(const ast::ImplicitAccessExpression&) -> void {}

auto TypeResolver::visit(const ast::StringExpression&) -> void {}

#define MAKE_PRIMITIVE_RESOLVER(NodeType, kind)                                           \
    auto TypeResolver::visit(const ast::NodeType& node) -> void {                         \
        last_type_.emplace(ctx_.pool.get_builtin_value_type(TypeKind::kind));             \
        if (!last_type_->has_resolved()) { last_type_->resolve<types::PrimitiveType>(); } \
        node.set_sema_type(*last_type_);                                                  \
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
MAKE_PRIMITIVE_RESOLVER(F32Expression, F32)
MAKE_PRIMITIVE_RESOLVER(F64Expression, F64)

auto TypeResolver::visit(const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(const ast::StructExpression&) -> void {}

auto TypeResolver::visit(const ast::ExplicitType&) -> void {}

auto TypeResolver::visit(const ast::TypeExpression&) -> void {}

auto TypeResolver::visit(const ast::UnionField&) -> void {}

auto TypeResolver::visit(const ast::UnionExpression&) -> void {}

auto TypeResolver::visit(const ast::WhileLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::BlockStatement&) -> void {}

auto TypeResolver::visit(const ast::BreakStatement&) -> void {}

auto TypeResolver::visit(const ast::ContinueStatement&) -> void {}

auto TypeResolver::visit(const ast::DeclStatement&) -> void {}

auto TypeResolver::visit(const ast::DeferStatement&) -> void {}

auto TypeResolver::visit(const ast::DiscardStatement&) -> void {}

auto TypeResolver::visit(const ast::ExpressionStatement&) -> void {}

auto TypeResolver::visit(const ast::ImportStatement&) -> void {}

auto TypeResolver::visit(const ast::ReturnStatement&) -> void {}

auto TypeResolver::visit(const ast::TestStatement&) -> void {}

auto TypeResolver::visit(const ast::UsingStatement&) -> void {}

} // namespace porpoise::sema

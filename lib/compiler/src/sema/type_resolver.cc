#include "sema/type_resolver.hh"
#include "sema/type.hh"

#include "ast/ast.hh"

namespace porpoise::sema {

namespace { constexpr auto IMMUTABLE = types::Key::Mutability::IMMUTABLE; } // namespace

auto TypeResolver::resolve_types(mod::Module& module, Context& ctx) -> mod::ModuleState {
    // Poisoned collection should flush the diagnostics
    if (module.state == mod::ModuleState::POISONED_SYMBOL_COLLECTION) {
        module.print_diagnostics(ctx.error_stream);
    }

    if (module.is_resolvable()) {
        ctx.inject_prelude();

        TypeResolver resolver{module, ctx};
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
    last_type_.emplace(ctx_.pool[{TypeKind::ARRAY, IMMUTABLE, null_terminated, size, item_type}]);
    if (!last_type_->has_resolved()) {
        last_type_->resolve<types::Array>(item_type, size, null_terminated);
    }
    array.set_sema_type(*last_type_);
}

auto TypeResolver::visit(const ast::CallArgument& arg) -> void {
    arg.match([this](const auto& a) { unwrap_and_accept(a); });
    arg.set_sema_type(*last_type_.take());
}

auto TypeResolver::visit(const ast::CallExpression& call) -> void {
    // The call can only yield a non-poison type if the function is valid
    call.get_function().accept(*this);
    mem::NonNull<Type> callee_type = last_type_.take();
    if (!callee_type->has_resolved() || callee_type->is_poison()) {
        call.set_sema_type(ctx_.get_poison());
        last_type_.emplace(ctx_.get_poison());
        return visit_list(call.get_arguments());
    }

    // Verify that the type in the function is callable and store the return type
    auto function_type = callee_type->as_opt<types::Function>();
    if (!function_type) {
        ctx_.diagnostics.emplace_back("Expression is not callable",
                                      Error::NON_CALLABLE_EXPRESSION,
                                      call.get_function().get_token());
        call.set_sema_type(ctx_.get_poison());
        last_type_.emplace(ctx_.get_poison());
        return visit_list(call.get_arguments());
    }
    call.get_function().set_sema_type(*callee_type);

    // Check the arity of the function against params before resetting last type
    const auto& params    = function_type->params;
    const auto& arguments = call.get_arguments();
    visit_list(arguments);

    if (arguments.size() != params.size()) {
        ctx_.diagnostics.emplace_back(
            fmt::format("Expected {} arguments but found {}", params.size(), arguments.size()),
            Error::ARITY_MISMATCH,
            call.get_token());
    }

    call.set_sema_type(function_type->return_type);
    last_type_.emplace(function_type->return_type);
}

auto TypeResolver::visit(const ast::DoWhileLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::Enumeration&) -> void {}

auto TypeResolver::visit(const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(const ast::ForLoopCapture&) -> void {}

auto TypeResolver::visit(const ast::ForLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::SelfParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionExpression&) -> void {}

auto TypeResolver::visit(const ast::IdentifierExpression& ident) -> void {
    const auto name   = ident.get_name();
    auto       symbol = ctx_.registry.lookup(table_stack_, name);

    // Check for an undeclared identifier and poison the ident
    if (!symbol) {
        ctx_.poison_node(ident,
                         fmt::format("Use of undeclared identifier '{}'", name),
                         Error::UNDECLARED_IDENTIFIER,
                         ident.get_token());
        return last_type_.emplace(ctx_.get_poison());
    }

    // Identifiers can be anything that they're symbol is allowed to be
    ident.set_sema_type(symbol->get_type());
    last_type_.emplace(symbol->get_type());
}

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

#define MAKE_PRIMITIVE_RESOLVER(NodeType, kind)                                         \
    auto TypeResolver::visit(const ast::NodeType& node) -> void {                       \
        last_type_.emplace(ctx_.pool[{TypeKind::kind, IMMUTABLE}]);                     \
        if (!last_type_->has_resolved()) { last_type_->resolve<types::BuiltinType>(); } \
        node.set_sema_type(*last_type_);                                                \
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

auto TypeResolver::visit(const ast::DiscardStatement& discard) -> void {
    discard.get_discarded().accept(*this);
}

auto TypeResolver::visit(const ast::ExpressionStatement& expr) -> void {
    expr.get_expression().accept(*this);
}

auto TypeResolver::visit(const ast::ImportStatement&) -> void {}

auto TypeResolver::visit(const ast::ReturnStatement&) -> void {}

auto TypeResolver::visit(const ast::TestStatement&) -> void {}

auto TypeResolver::visit(const ast::UsingStatement&) -> void {}

} // namespace porpoise::sema

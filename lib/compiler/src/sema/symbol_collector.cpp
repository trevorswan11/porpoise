#include <utility>

#include "sema/symbol_collector.hpp"

#include "ast/ast.hpp"
#include "ast/visitor.hpp"

namespace porpoise::sema {

// Few expressions are evaluated directly on this pass
#define MAKE_COLLECTOR_NOOPS(X)  \
    X(ArrayExpression)           \
    X(CallArgument)              \
    X(CallExpression)            \
    X(DoWhileLoopExpression)     \
    X(SelfParameter)             \
    X(FunctionParameter)         \
    X(ForLoopCapture)            \
    X(ForLoopExpression)         \
    X(IdentifierExpression)      \
    X(IfExpression)              \
    X(IndexExpression)           \
    X(InfiniteLoopExpression)    \
    X(AssignmentExpression)      \
    X(BinaryExpression)          \
    X(DotExpression)             \
    X(RangeExpression)           \
    X(Initializer)               \
    X(InitializerExpression)     \
    X(MatchArm)                  \
    X(MatchExpression)           \
    X(ReferenceExpression)       \
    X(DereferenceExpression)     \
    X(UnaryExpression)           \
    X(ImplicitAccessExpression)  \
    X(StringExpression)          \
    X(I32Expression)             \
    X(I64Expression)             \
    X(ISizeExpression)           \
    X(U32Expression)             \
    X(U64Expression)             \
    X(USizeExpression)           \
    X(U8Expression)              \
    X(F32Expression)             \
    X(F64Expression)             \
    X(BoolExpression)            \
    X(ScopeResolutionExpression) \
    X(ExplicitType)              \
    X(TypeExpression)            \
    X(WhileLoopExpression)

#define GENERATE_COLLECTOR_NOOP(NodeType) GENERATE_VISITOR_NOOP(SymbolCollector, NodeType)
MAKE_COLLECTOR_NOOPS(GENERATE_COLLECTOR_NOOP)

auto SymbolCollector::visit(const ast::EnumExpression& enum_expr) -> void {
    const auto scope_idx = visit_scope(
        TypeKind::ENUM,
        IterPair{enum_expr.get_enumerations(), [this](const auto& field) { visit(field); }},
        IterPair{enum_expr.get_members(), [this](const auto& decl) { visit(*decl); }});
    last_type_->set_symbol_table(scope_idx);
}

// This is assumed to be invoked only by the enum visitor
auto SymbolCollector::visit(const ast::Enumeration& enumeration) -> void {
    try_declare(enumeration.get_ident().get_name(), &enumeration);
}

auto SymbolCollector::visit(const ast::FunctionExpression& fn) -> void {
    types::Key key{TypeKind::FUNCTION, false};

    // Parameters belong to the parent scope
    if (fn.has_self()) { try_declare(fn.get_self().get_ident().get_name(), &fn.get_self()); }
    for (const auto& param : fn.get_parameters()) {
        try_declare(param.get_ident().get_name(), &param);
    }

    // The new table index has to be handled differently than a normal scope
    Optional<usize> new_idx;
    if (fn.has_body()) {
        new_idx.emplace(registry_.create());
        const Scope s{table_stack_, *new_idx, table_idx_};
        visit(fn.get_body());
    }

    if (new_idx) { key.emplace_idx(*new_idx); }
    last_type_.emplace(pool_[key]);
    if (new_idx) { last_type_->set_symbol_table(*new_idx); }
}

auto SymbolCollector::visit(const ast::StructExpression& struct_expr) -> void {
    const auto scope_idx = visit_scope(
        struct_expr, TypeKind::STRUCT, [this](const auto& field) { field->accept(*this); });
    last_type_->set_symbol_table(scope_idx);
}

auto SymbolCollector::visit(const ast::UnionExpression& union_expr) -> void {
    const auto scope_idx =
        visit_scope(TypeKind::UNION,
                    IterPair{union_expr.get_fields(), [this](const auto& field) { visit(field); }},
                    IterPair{union_expr.get_members(), [this](const auto& decl) { visit(*decl); }});
    last_type_->set_symbol_table(scope_idx);
}

// This is assumed to be invoked only by the union visitor
auto SymbolCollector::visit(const ast::UnionField& field) -> void {
    try_declare(field.get_ident().get_name(), &field);
}

auto SymbolCollector::visit(const ast::BlockStatement& block) -> void {
    if (table_idx_ == 0) {
        diagnostics_.emplace_back("Cannot have block at the top level",
                                  Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                  block.get_token());
    }
    for (const auto& stmt : block) { stmt->accept(*this); }
}

auto SymbolCollector::visit(const ast::DeclStatement& decl) -> void {
    // Only move on to value inspection is the node
    const auto name = decl.get_ident().get_name();
    if (!try_declare(name, &decl)) { return; }

    if (decl.has_modifier(ast::DeclModifiers::PUBLIC)) {
        registry_.get_from(table_idx_, name).mark_public();
    }
    if (!decl.has_value()) { return; }

    // Valued decls should be evaluated to get shallow types
    const auto& expr = decl.get_value();
    if (expr.any<ast::EnumExpression,
                 ast::FunctionExpression,
                 ast::UnionExpression,
                 ast::StructExpression>()) {
        static constexpr auto expr_name = [](ast::NodeKind kind) {
            switch (kind) {
            case ast::NodeKind::ENUM_EXPRESSION:     return "enum";
            case ast::NodeKind::FUNCTION_EXPRESSION: return "function";
            case ast::NodeKind::UNION_EXPRESSION:    return "union";
            case ast::NodeKind::STRUCT_EXPRESSION:   return "struct";
            default:                                 std::unreachable();
            }
        };

        if (decl.has_modifier(ast::DeclModifiers::CONSTEXPR)) {
            diagnostics_.emplace_back(fmt::format("Top level {}s are implicitly compile time known",
                                                  expr_name(expr.get_kind())),
                                      Error::REDUNDANT_CONSTEXPR,
                                      decl.get_token());
        } else if (!decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            diagnostics_.emplace_back(
                fmt::format("Top level {}s must be marked const at the top level",
                            expr_name(expr.get_kind())),
                Error::ILLEGAL_NON_CONST_STATEMENT,
                decl.get_token());
        }
    }

    // Functions cannot be used as valued stubs
    if (expr.is<ast::FunctionExpression>()) {
        const auto& func = ast::Node::as<ast::FunctionExpression>(expr);
        if (!func.has_body()) {
            diagnostics_.emplace_back(
                fmt::format("Functions cannot be used as valued stubs in declarations"),
                Error::FUNCTION_DECLARATION_MISSING_BODY,
                decl.get_token());
        }
    }

    expr.accept(*this);
    if (last_type_) { registry_.get_from(table_idx_, name).emplace_type(*last_type_.take()); }
}

#define ILLEGAL_COLLECTOR_TOP_LEVEL(NodeType, stringified_node)                        \
    auto SymbolCollector::visit(const NodeType& node) -> void {                        \
        diagnostics_.emplace_back("Cannot have " stringified_node " at the top level", \
                                  Error::ILLEGAL_TOP_LEVEL_STATEMENT,                  \
                                  node.get_token());                                   \
    }

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DeferStatement, "defer")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DiscardStatement, "discard")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::ExpressionStatement, "expression")

auto SymbolCollector::visit(const ast::ImportStatement& import_stmt) -> void {
    assert(table_stack_.size() == 1 && "Import not at top level");
    if (registry_.get(table_idx_).is_module()) { import_stmt.mark_public(); }
    const auto name = import_stmt.match(Overloaded{
        [](const ast::LibraryImport& module) {
            return module.has_alias() ? module.get_alias().get_name()
                                      : module.get_name().get_name();
        },
        [](const ast::FileImport& user) { return user.get_alias().get_name(); },
    });
    try_result(registry_.insert_into(table_idx_, name, &import_stmt));
}

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::JumpStatement, "jump")

auto SymbolCollector::visit(const ast::ModuleStatement& module_stmt) -> void {
    auto& table = registry_.get(table_idx_);
    if (first_node_) {
        table.indicate_module();
    } else if (table.is_module()) {
        diagnostics_.emplace_back("Only one module statement is allowed per file",
                                  Error::DUPLICATE_MODULE_STATEMENT,
                                  module_stmt.get_token());
    } else {
        diagnostics_.emplace_back("Module indicator must be first statement of file",
                                  Error::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                  module_stmt.get_token());
    }
}

auto SymbolCollector::visit(const ast::UsingStatement& using_stmt) -> void {
    if (registry_.get(table_idx_).is_module()) { using_stmt.mark_public(); }
    try_declare(using_stmt.get_alias().get_name(), &using_stmt);
}

} // namespace porpoise::sema

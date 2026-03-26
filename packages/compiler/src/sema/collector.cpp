#include "sema/collector.hpp"

#include "ast/ast.hpp"

namespace porpoise::sema {

// Few expressions are evaluated directly on this pass
#define COLLECTOR_NOOP(NodeType) \
    auto SymbolCollector::visit(const ast::NodeType&) -> void {}

COLLECTOR_NOOP(ArrayExpression)
COLLECTOR_NOOP(CallExpression)
COLLECTOR_NOOP(DoWhileLoopExpression)
COLLECTOR_NOOP(ForLoopExpression)
COLLECTOR_NOOP(FunctionExpression)
COLLECTOR_NOOP(IdentifierExpression)
COLLECTOR_NOOP(IfExpression)
COLLECTOR_NOOP(IndexExpression)
COLLECTOR_NOOP(InfiniteLoopExpression)
COLLECTOR_NOOP(AssignmentExpression)
COLLECTOR_NOOP(BinaryExpression)
COLLECTOR_NOOP(DotExpression)
COLLECTOR_NOOP(RangeExpression)
COLLECTOR_NOOP(MatchExpression)
COLLECTOR_NOOP(ReferenceExpression)
COLLECTOR_NOOP(DereferenceExpression)
COLLECTOR_NOOP(ImplicitAccessExpression)
COLLECTOR_NOOP(UnaryExpression)
COLLECTOR_NOOP(StringExpression)
COLLECTOR_NOOP(SignedIntegerExpression)
COLLECTOR_NOOP(SignedLongIntegerExpression)
COLLECTOR_NOOP(ISizeIntegerExpression)
COLLECTOR_NOOP(UnsignedIntegerExpression)
COLLECTOR_NOOP(UnsignedLongIntegerExpression)
COLLECTOR_NOOP(USizeIntegerExpression)
COLLECTOR_NOOP(ByteExpression)
COLLECTOR_NOOP(FloatExpression)
COLLECTOR_NOOP(DoubleExpression)
COLLECTOR_NOOP(BoolExpression)
COLLECTOR_NOOP(ScopeResolutionExpression)
COLLECTOR_NOOP(TypeExpression)
COLLECTOR_NOOP(WhileLoopExpression)

auto SymbolCollector::visit(const ast::EnumExpression& enum_expr) -> void {
    visit_scope(enum_expr, TypeKind::ENUM, [this](const auto& field) { visit(field); });
}

// This is assumed to be invoked only by the enum visitor
auto SymbolCollector::visit(const ast::Enumeration& enumeration) -> void {
    try_declare(enumeration.get_ident().get_name(), &enumeration);
}

auto SymbolCollector::visit(const ast::StructExpression& struct_expr) -> void {
    visit_scope(struct_expr, TypeKind::STRUCT, [this](const auto& field) { field->accept(*this); });
}

auto SymbolCollector::visit(const ast::UnionExpression& union_expr) -> void {
    visit_scope(union_expr, TypeKind::UNION, [this](const auto& field) { visit(field); });
}

// This is assumed to be invoked only by the union visitor
auto SymbolCollector::visit(const ast::UnionField& field) -> void {
    try_declare(field.get_ident().get_name(), &field);
}

#define ILLEGAL_COLLECTOR_TOP_LEVEL(NodeType, stringified_node)                        \
    auto SymbolCollector::visit(const NodeType& node) -> void {                        \
        diagnostics_.emplace_back("Cannot have " stringified_node " at the top level", \
                                  Error::ILLEGAL_TOP_LEVEL_STATEMENT,                  \
                                  node.get_token());                                   \
    }

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::BlockStatement, "block")

auto SymbolCollector::visit(const ast::DeclStatement& decl) -> void {
    // Only move on to value inspection is the node
    const auto name = decl.get_ident().get_name();
    if (!try_declare(name, &decl)) { return; }

    auto& symbol = registry_.get_from(table_idx_, name);
    if (decl.has_modifier(ast::DeclModifiers::PUBLIC)) { symbol.mark_public(); }

    // Attach bubbled types to the symbol just created
    if (decl.has_value()) {
        decl.get_value().accept(*this);

        if (last_type_) {
            symbol.emplace_type(*last_type_);
            last_type_.reset();
        }
    }
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

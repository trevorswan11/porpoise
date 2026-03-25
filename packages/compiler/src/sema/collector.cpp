#include <utility>

#include "sema/collector.hpp"

#include "ast/ast.hpp"
#include "common.hpp"

namespace porpoise::sema {

// Few expressions are evaluated directly on this pass
#define COLLECTOR_NOOP(NodeType) \
    auto SymbolCollector::visit(const ast::NodeType&) -> void { std::unreachable(); }

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

auto SymbolCollector::visit(const ast::EnumExpression& enum_expr) -> void { TODO(enum_expr); }

auto SymbolCollector::visit(const ast::StructExpression& struct_expr) -> void { TODO(struct_expr); }

auto SymbolCollector::visit(const ast::UnionExpression& union_expr) -> void { TODO(union_expr); }

#define ILLEGAL_COLLECTOR_TOP_LEVEL(NodeType, stringified_node)                        \
    auto SymbolCollector::visit(const NodeType& node) -> void {                        \
        diagnostics_.emplace_back("Cannot have " stringified_node " at the top level", \
                                  SemaError::ILLEGAL_TOP_LEVEL_STATEMENT,              \
                                  node.get_token());                                   \
    }

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::BlockStatement, "block")

auto SymbolCollector::visit(const ast::DeclStatement& decl) -> void {
    auto result = registry_.insert_into(table_idx_, decl.get_ident().get_name(), &decl);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DeferStatement, "defer")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DiscardStatement, "discard")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::ExpressionStatement, "expression")

auto SymbolCollector::visit(const ast::ImportStatement& import_stmt) -> void {
    if (registry_.get(table_idx_).is_module()) { import_stmt.mark_public(); }
    const auto name   = import_stmt.match(Overloaded{
        [](const ast::LibraryImport& module) {
            return module.has_alias() ? module.get_alias().get_name()
                                        : module.get_name().get_name();
        },
        [](const ast::FileImport& user) { return user.get_alias().get_name(); },
    });
    auto       result = registry_.insert_into(table_idx_, name, &import_stmt);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::JumpStatement, "jump")

auto SymbolCollector::visit(const ast::ModuleStatement& module_stmt) -> void {
    auto& table = registry_.get(table_idx_);
    if (first_node_) {
        table.indicate_module();
    } else if (table.is_module()) {
        diagnostics_.emplace_back("Only one module statement is allowed per file",
                                  SemaError::DUPLICATE_MODULE_STATEMENT,
                                  module_stmt.get_token());
    } else {
        diagnostics_.emplace_back("Module indicator must be first statement of file",
                                  SemaError::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                  module_stmt.get_token());
    }
}

auto SymbolCollector::visit(const ast::UsingStatement& using_stmt) -> void {
    if (registry_.get(table_idx_).is_module()) { using_stmt.mark_public(); }
    auto result = registry_.insert_into(table_idx_, using_stmt.get_alias().get_name(), &using_stmt);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

} // namespace porpoise::sema

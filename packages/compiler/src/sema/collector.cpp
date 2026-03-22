#include "sema/collector.hpp"

#include "ast/ast.hpp"
#include "variant.hpp"

namespace porpoise::sema {

auto SymbolCollector::collect(ast::ASTView ast) -> std::pair<SymbolTable, Diagnostics> {
    SymbolTable     table;
    Diagnostics     diagnostics;
    SymbolCollector collector{table, diagnostics};

    for (const auto& node : ast) {
        node->accept(collector);
        collector.first_node_ = false;
    }
    return {std::move(table), std::move(diagnostics)};
}

// No expressions are evaluated directly on this pass
#define COLLECTOR_NOOP(NodeType) \
    auto SymbolCollector::visit(const ast::NodeType&) -> void {}
FOREACH_AST_EXPR(COLLECTOR_NOOP)

#define ILLEGAL_COLLECTOR_TOP_LEVEL(NodeType, stringified_node)                        \
    auto SymbolCollector::visit(const NodeType& node) -> void {                        \
        diagnostics_.emplace_back("Cannot have " stringified_node " at the top level", \
                                  SemaError::ILLEGAL_TOP_LEVEL_STATEMENT,              \
                                  node.get_token());                                   \
    }

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::BlockStatement, "block")

auto SymbolCollector::visit(const ast::DeclStatement& decl) -> void {
    auto result = table_.insert(decl.get_ident().get_name(), &decl);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DeferStatement, "defer")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::DiscardStatement, "discard")
ILLEGAL_COLLECTOR_TOP_LEVEL(ast::ExpressionStatement, "expression")

auto SymbolCollector::visit(const ast::ImportStatement& import_stmt) -> void {
    if (table_.is_module()) { import_stmt.mark_public(); }
    const auto name   = import_stmt.match(Overloaded{
        [](const ast::ModuleImport& module) {
            return module.has_alias() ? module.get_alias().get_name()
                                        : module.get_name().get_name();
        },
        [](const ast::UserImport& user) { return user.get_alias().get_name(); },
    });
    auto       result = table_.insert(name, &import_stmt);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

ILLEGAL_COLLECTOR_TOP_LEVEL(ast::JumpStatement, "jump")

auto SymbolCollector::visit(const ast::ModuleStatement& module_stmt) -> void {
    if (first_node_) {
        table_.indicate_module();
    } else if (table_.is_module()) {
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
    if (table_.is_module()) { using_stmt.mark_public(); }
    auto result = table_.insert(using_stmt.get_alias().get_name(), &using_stmt);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

} // namespace porpoise::sema

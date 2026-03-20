#include <ranges>

#include "sema/collector.hpp"

#include "ast/ast.hpp"
#include "variant.hpp"

namespace porpoise::sema {

auto SymbolCollector::collect(ast::ASTView ast) -> std::pair<SymbolTable, Diagnostics> {
    SymbolTable     table;
    Diagnostics     diagnostics;
    SymbolCollector collector{table, diagnostics};

    for (const auto& [node, i] : std::views::zip(ast, std::views::iota(0))) {
        if (node->is<ast::ModuleStatement>()) {
            // Tell the symbol table that its a module now to prevent confusion in pass 2
            if (i == 0) {
                table.indicate_module();
            } else {
                diagnostics.emplace_back("Module indicator must be first statement of file",
                                         SemaError::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                         node->get_token());
            }
            continue;
        }
        node->accept(collector);
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
COLLECTOR_NOOP(ModuleStatement)

auto SymbolCollector::visit(const ast::UsingStatement& using_stmt) -> void {
    auto result = table_.insert(using_stmt.get_alias().get_name(), &using_stmt);
    if (!result) { diagnostics_.emplace_back(result.error()); }
}

} // namespace porpoise::sema

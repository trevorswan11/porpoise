#include "sema/symbol_collector.hpp"

#include "ast/ast.hpp"
#include "ast/visitor.hpp"

namespace porpoise::sema {

// Many terminal expressions are skipped on this pass
#define MAKE_COLLECTOR_NOOPS(X)  \
    X(IdentifierExpression)      \
    X(DotExpression)             \
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
    X(VoidExpression)            \
    X(ScopeResolutionExpression) \
    X(ExplicitType)              \
    X(TypeExpression)

#define GENERATE_COLLECTOR_NOOP(NodeType) GENERATE_VISITOR_NOOP(SymbolCollector, NodeType)
MAKE_COLLECTOR_NOOPS(GENERATE_COLLECTOR_NOOP)

auto SymbolCollector::visit(const ast::ArrayExpression& array) -> void {
    const auto g = in_expr_scope_.guard();
    visit_list(array.get_items());
}

auto SymbolCollector::visit(const ast::CallArgument& arg) -> void {
    arg.match([this](const auto& a) { unwrap_and_accept(a); });
}

auto SymbolCollector::visit(const ast::CallExpression& call) -> void {
    const auto g = in_expr_scope_.guard();
    visit_list(call.get_arguments());
}

auto SymbolCollector::visit(const ast::DoWhileLoopExpression& do_while) -> void {
    const auto g   = loop_guard();
    const auto idx = visit_scopes(
        TypeKind::BLOCK,
        IterPair{do_while.get_block(), [this](const auto& stmt) { stmt->accept(*this); }});
    last_type_->set_symbol_table_idx(idx);
    do_while.set_sema_type(*last_type_.take());
}

// This is assumed to be invoked only by the enum visitor
auto SymbolCollector::visit(const ast::Enumeration& enumeration) -> void {
    try_declare(enumeration.get_ident().get_name(), &enumeration);
}

auto SymbolCollector::visit(const ast::EnumExpression& enum_expr) -> void {
    const auto scope_idx = visit_scopes(
        TypeKind::ENUM,
        IterPair{enum_expr.get_enumerations(), [this](const auto& field) { visit(field); }},
        IterPair{enum_expr.get_members(), [this](const auto& decl) { visit(*decl); }});
    last_type_->set_symbol_table_idx(scope_idx);
    enum_expr.set_sema_type(*last_type_);
}

auto SymbolCollector::visit(const ast::ForLoopCapture& capture) -> void {
    if (capture.is_valued()) { try_declare(capture.get_valued().get_ident().get_name(), &capture); }
}

auto SymbolCollector::visit(const ast::ForLoopExpression& for_expr) -> void {
    // The guard shouldn't enclose the else clause
    const auto g_expr = in_expr_scope_.guard();
    usize      new_idx;
    {
        new_idx = registry_.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto g_loop = in_loop_scope_.guard();
        visit_list(for_expr.get_captures());
        visit_list(for_expr.get_block());
    }
    if (for_expr.has_non_break()) { for_expr.get_non_break().accept(*this); }

    last_type_.emplace(pool_[{TypeKind::BLOCK, false, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    for_expr.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::SelfParameter& self) -> void {
    try_declare(self.get_ident().get_name(), &self);
}

auto SymbolCollector::visit(const ast::FunctionParameter& param) -> void {
    try_declare(param.get_ident().get_name(), &param);
}

auto SymbolCollector::visit(const ast::FunctionExpression& fn) -> void {
    const auto  new_idx = registry_.create();
    const Scope s{table_stack_, new_idx, table_idx_};
    const auto  g = fn_guard();

    // Parameters belong to the parent scope
    if (fn.has_self()) { visit(fn.get_self()); }
    visit_list(fn.get_parameters());
    if (fn.has_body()) { visit_list(fn.get_body()); }

    last_type_.emplace(pool_[{TypeKind::FUNCTION, false, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    fn.set_sema_type(*last_type_);
}

auto SymbolCollector::visit(const ast::IfExpression& if_expr) -> void {
    const auto g = in_expr_scope_.guard();
    if_expr.get_consequence().accept(*this);
    if (if_expr.has_alternate()) { if_expr.get_alternate().accept(*this); }
}

auto SymbolCollector::visit(const ast::IndexExpression& index) -> void {
    const auto g = in_expr_scope_.guard();
    index.get_index().accept(*this);
}

auto SymbolCollector::visit(const ast::InfiniteLoopExpression& loop) -> void {
    const auto g = loop_guard();
    const auto idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{loop.get_block(), [this](const auto& stmt) { stmt->accept(*this); }});
    last_type_->set_symbol_table_idx(idx);
    loop.set_sema_type(*last_type_.take());
}

#define MAKE_INFIX_COLLECTOR(NodeType)                          \
    auto SymbolCollector::visit(const NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                  \
        node.get_lhs().accept(*this);                           \
        node.get_rhs().accept(*this);                           \
    }

MAKE_INFIX_COLLECTOR(ast::RangeExpression)
MAKE_INFIX_COLLECTOR(ast::AssignmentExpression)
MAKE_INFIX_COLLECTOR(ast::BinaryExpression)

auto SymbolCollector::visit(const ast::Initializer& init) -> void {
    init.get_value().accept(*this);
}

auto SymbolCollector::visit(const ast::InitializerExpression& init) -> void {
    const auto g = in_expr_scope_.guard();
    if (init.has_initializers()) { visit_list(init.get_initializers()); }
}

auto SymbolCollector::visit(const ast::MatchArm& arm) -> void {
    const auto  new_idx = registry_.create();
    const Scope s{table_stack_, new_idx, table_idx_};

    if (arm.has_capture_clause() && arm.is_explicit_capture()) {
        try_declare(arm.get_explicit_capture().get_name(), &arm);
    }
    arm.get_dispatch().accept(*this);

    last_type_.emplace(pool_[{TypeKind::BLOCK, false, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    arm.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::MatchExpression& match) -> void {
    const auto g = in_expr_scope_.guard();
    visit_list(match.get_arms());
    if (match.has_catch_all()) { match.get_catch_all().accept(*this); }
}

#define MAKE_PREFIX_COLLECTOR(NodeType)                         \
    auto SymbolCollector::visit(const NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                  \
        node.get_rhs().accept(*this);                           \
    }

MAKE_PREFIX_COLLECTOR(ast::ReferenceExpression)
MAKE_PREFIX_COLLECTOR(ast::DereferenceExpression)
MAKE_PREFIX_COLLECTOR(ast::UnaryExpression)

auto SymbolCollector::visit(const ast::StructExpression& struct_expr) -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::STRUCT,
                     IterPair{struct_expr, [this](const auto& field) { field->accept(*this); }});
    last_type_->set_symbol_table_idx(scope_idx);
    struct_expr.set_sema_type(*last_type_);
}

// This is assumed to be invoked only by the union visitor
auto SymbolCollector::visit(const ast::UnionField& field) -> void {
    try_declare(field.get_ident().get_name(), &field);
}

auto SymbolCollector::visit(const ast::UnionExpression& union_expr) -> void {
    const auto scope_idx = visit_scopes(
        TypeKind::UNION,
        IterPair{union_expr.get_fields(), [this](const auto& field) { visit(field); }},
        IterPair{union_expr.get_members(), [this](const auto& decl) { visit(*decl); }});
    last_type_->set_symbol_table_idx(scope_idx);
    union_expr.set_sema_type(*last_type_);
}

auto SymbolCollector::visit(const ast::WhileLoopExpression& while_expr) -> void {
    // The guard shouldn't enclose the else clause
    const auto g_expr = in_expr_scope_.guard();
    usize      idx;
    {
        const auto g_loop = in_loop_scope_.guard();
        idx               = visit_scopes(
            TypeKind::BLOCK,
            IterPair{while_expr.get_block(), [this](const auto& stmt) { stmt->accept(*this); }});
    }
    if (while_expr.has_non_break()) { while_expr.get_non_break().accept(*this); }

    last_type_->set_symbol_table_idx(idx);
    while_expr.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::BlockStatement& block) -> void {
    if (!in_expr_scope_ && table_stack_.size() == 1) {
        diagnostics_.emplace_back("Cannot have block at the top level",
                                  Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                  block.get_token());
    }

    const auto scope_idx = visit_scopes(
        TypeKind::BLOCK, IterPair{block, [this](const auto& stmt) { stmt->accept(*this); }});
    last_type_->set_symbol_table_idx(scope_idx);
    block.set_sema_type(*last_type_);
}

auto SymbolCollector::visit(const ast::DeclStatement& decl) -> void {
    // We can stop analyzing early if there's no value
    const auto name = decl.get_ident().get_name();
    try_declare(name, &decl);
    if (!decl.has_value()) { return; }

    // Valued decls should be evaluated to get shallow types
    const auto& expr = decl.get_value();
    if (expr.any<ast::EnumExpression,
                 ast::FunctionExpression,
                 ast::UnionExpression,
                 ast::StructExpression>()) {
        // Certain values cannot be constexpr or non-const to reduce mental overhead
        if (decl.has_modifier(ast::DeclModifiers::CONSTEXPR)) {
            diagnostics_.emplace_back(
                fmt::format("Top level {}s are implicitly compile time known", expr.display_name()),
                Error::REDUNDANT_CONSTEXPR,
                decl.get_token());
        } else if (!decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            diagnostics_.emplace_back(
                fmt::format("Top level {}s must be marked const at the top level",
                            expr.display_name()),
                Error::ILLEGAL_NON_CONST_STATEMENT,
                decl.get_token());
        }
    }

    expr.accept(*this);
    if (last_type_) { registry_.get_from(table_idx_, name).emplace_type(*last_type_.take()); }
}

auto SymbolCollector::visit(const ast::DeferStatement& defer) -> void {
    if (!in_function_scope_) {
        diagnostics_.emplace_back("Cannot have defer outside of a function's scope",
                                  Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                  defer.get_token());
    }
    defer.get_deferred().accept(*this);
}

auto SymbolCollector::visit(const ast::DiscardStatement& discard) -> void {
    discard.get_discarded().accept(*this);
}

auto SymbolCollector::visit(const ast::ExpressionStatement& expr) -> void {
    expr.get_expression().accept(*this);
}

auto SymbolCollector::visit(const ast::ImportStatement& import_stmt) -> void {
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

auto SymbolCollector::visit(const ast::JumpStatement& node) -> void {
    using syntax::TokenType;
    switch (node.get_token().type) {
    case TokenType::RETURN:
        if (!in_function_scope_) {
            diagnostics_.emplace_back("Cannot return outside of a function",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      node.get_token());
        }
        break;
    case TokenType::BREAK:
    case TokenType::CONTINUE:
        if (!in_loop_scope_) {
            diagnostics_.emplace_back("Cannot continue or break outside of a loop",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      node.get_token());
        }
        break;
    default: std::unreachable();
    }
    if (node.has_expression()) { node.get_expression().accept(*this); }
}

auto SymbolCollector::visit(const ast::ModuleStatement& module_stmt) -> void {
    auto& table = registry_.get(table_idx_);
    if (first_node_) { return table.indicate_module(); }

    // Module statements are highly restricted in terms of usage and location
    if (table.is_module()) {
        diagnostics_.emplace_back("Only one module statement is allowed per file",
                                  Error::DUPLICATE_MODULE_STATEMENT,
                                  module_stmt.get_token());
    } else {
        diagnostics_.emplace_back("Module indicator must be first statement of file",
                                  Error::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                  module_stmt.get_token());
    }
}

auto SymbolCollector::visit(const ast::TestStatement& test) -> void {
    if (table_idx_ != 0) {
        diagnostics_.emplace_back("Tests must be at the topmost level of a file",
                                  Error::ILLEGAL_TEST_LOCATION,
                                  test.get_token());
    }

    // Not a symbol so don't push to the table, track in node instead
    const auto scope_idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{test.get_block(), [this](const auto& decl) { decl->accept(*this); }});
    last_type_->set_symbol_table_idx(scope_idx);
    test.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::UsingStatement& using_stmt) -> void {
    if (registry_.get(table_idx_).is_module()) { using_stmt.mark_public(); }
    try_declare(using_stmt.get_alias().get_name(), &using_stmt);
}

} // namespace porpoise::sema

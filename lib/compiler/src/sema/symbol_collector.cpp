#include "sema/symbol_collector.hpp"

#include "ast/ast.hpp"
#include "ast/visitor.hpp"

namespace porpoise::sema {

auto SymbolCollector::collect_symbols(mod::Module& module, const CollectorCtx& ctx)
    -> mod::ModuleState {
    if (module.state < mod::ModuleState::SYMBOLS_COLLECTED && !module.root_table_idx) {
        module.root_table_idx.emplace(ctx.registry.create());

        SymbolCollector collector{module, ctx};
        for (const auto& node : module.tree) {
            node->accept(collector);
            collector.pass_first();
        }

        if (!ctx.diagnostics.empty()) { return module.error_out(std::move(ctx.diagnostics)); }
        module.state = mod::ModuleState::SYMBOLS_COLLECTED;
    }
    return module.state;
}

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
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto g_loop = in_loop_scope_.guard();
        visit_list(for_expr.get_captures());
        visit_list(for_expr.get_block());
    }
    if (for_expr.has_non_break()) { for_expr.get_non_break().accept(*this); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, false, new_idx}]);
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
    const auto  new_idx = ctx_.registry.create();
    const Scope s{table_stack_, new_idx, table_idx_};
    const auto  g = fn_guard();

    // Parameters belong to the parent scope
    if (fn.has_self()) { visit(fn.get_self()); }
    visit_list(fn.get_parameters());
    if (fn.has_body()) { visit_list(fn.get_body()); }

    last_type_.emplace(ctx_.pool[{TypeKind::FUNCTION, false, new_idx}]);
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

auto SymbolCollector::visit(const ast::LabelExpression& label) -> void {
    const auto g = label_guard();
    try_declare(label.get_name().get_name(), &label);
    label.match([this](const auto& b) { visit(*b); });
}

auto SymbolCollector::visit(const ast::MatchArm& arm) -> void {
    const auto  new_idx = ctx_.registry.create();
    const Scope s{table_stack_, new_idx, table_idx_};

    if (arm.has_capture_clause() && arm.is_explicit_capture()) {
        try_declare(arm.get_explicit_capture().get_name(), &arm);
    }
    arm.get_dispatch().accept(*this);

    last_type_.emplace(ctx_.pool[{TypeKind::MATCH_ARM, false, new_idx}]);
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
    usize      new_idx;
    {
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto g_loop = in_loop_scope_.guard();
        visit_list(while_expr.get_block());
    }
    if (while_expr.has_non_break()) { while_expr.get_non_break().accept(*this); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, false, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    while_expr.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::BlockStatement& block) -> void {
    if (!in_expr_scope_ && table_stack_.size() == 1) {
        ctx_.diagnostics.emplace_back("Cannot have block at the top level",
                                      Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                      block.get_token());
    }

    const auto scope_idx = visit_scopes(
        TypeKind::BLOCK, IterPair{block, [this](const auto& stmt) { stmt->accept(*this); }});
    last_type_->set_symbol_table_idx(scope_idx);
    block.set_sema_type(*last_type_.take());
}

auto SymbolCollector::visit(const ast::BreakStatement& break_stmt) -> void {
    if (!in_loop_scope_ && !in_label_scope_) {
        ctx_.diagnostics.emplace_back("Cannot break outside of a loop or label",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      break_stmt.get_token());
    }
    if (break_stmt.has_expression()) { break_stmt.get_expression().accept(*this); }
}

auto SymbolCollector::visit(const ast::ContinueStatement& continue_stmt) -> void {
    if (!in_loop_scope_) {
        ctx_.diagnostics.emplace_back("Cannot continue outside of a loop",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      continue_stmt.get_token());
    }
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
            ctx_.diagnostics.emplace_back(
                fmt::format("Top level {}s are implicitly compile time known", expr.display_name()),
                Error::REDUNDANT_CONSTEXPR,
                decl.get_token());
        } else if (!decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            ctx_.diagnostics.emplace_back(
                fmt::format("Top level {}s must be marked const at the top level",
                            expr.display_name()),
                Error::ILLEGAL_NON_CONST_STATEMENT,
                decl.get_token());
        }
    }

    expr.accept(*this);
    if (last_type_) { ctx_.registry.get_from(table_idx_, name).emplace_type(*last_type_.take()); }
}

auto SymbolCollector::visit(const ast::DeferStatement& defer) -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot have defer outside of a function's scope",
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
    if (ctx_.registry.get(table_idx_).is_module()) { import_stmt.mark_public(); }

    auto [alias, mod_result] = import_stmt.match(Overloaded{
        [this](const ast::LibraryImport& module) {
            const auto name =
                module.has_alias() ? module.get_alias().get_name() : module.get_name().get_name();
            auto mod = ctx_.modules.try_get_true_module(module.get_name().materialize());
            return std::pair{name, mod};
        },
        [this](const ast::FileImport& user) {
            const auto name = user.get_alias().get_name();
            auto       mod  = ctx_.modules.try_get_file_module(user.get_file().get_value(),
                                                        collecting_.parent_path);
            return std::pair{name, mod};
        },
    });

    // Only set the table index if the module exists
    opt::Option<mod::Module&> imported_mod;
    if (!mod_result) {
        // The token is retrieved here to avoid copying it on success
        ctx_.diagnostics.emplace_back(
            std::move(mod_result.error()),
            import_stmt.match(Overloaded{
                [](const ast::LibraryImport& module) { return module.get_name().get_token(); },
                [](const ast::FileImport& user) { return user.get_file().get_token(); },
            }));
    } else {
        imported_mod.emplace(**mod_result);
        Diagnostics diags;
        collect_symbols(*imported_mod, ctx_.copy(diags));
    }
    try_result(
        ctx_.registry.insert_into(table_idx_, alias, SymbolicImport{&import_stmt, imported_mod}));
}

auto SymbolCollector::visit(const ast::ModuleStatement& module_stmt) -> void {
    auto& table = ctx_.registry.get(table_idx_);
    if (first_node_) { return table.indicate_module(); }

    // Module statements are highly restricted in terms of usage and location
    if (table.is_module()) {
        ctx_.diagnostics.emplace_back("Only one module statement is allowed per file",
                                      Error::DUPLICATE_MODULE_STATEMENT,
                                      module_stmt.get_token());
    } else {
        ctx_.diagnostics.emplace_back("Module indicator must be first statement of file",
                                      Error::ILLEGAL_MODULE_STATEMENT_LOCATION,
                                      module_stmt.get_token());
    }
}

auto SymbolCollector::visit(const ast::ReturnStatement& return_stmt) -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot return outside of a function",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      return_stmt.get_token());
    }
    if (return_stmt.has_expression()) { return_stmt.get_expression().accept(*this); }
}

auto SymbolCollector::visit(const ast::TestStatement& test) -> void {
    if (table_idx_ != 0) {
        ctx_.diagnostics.emplace_back("Tests must be at the topmost level of a file",
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
    if (ctx_.registry.get(table_idx_).is_module()) { using_stmt.mark_public(); }
    try_declare(using_stmt.get_alias().get_name(), &using_stmt);
}

} // namespace porpoise::sema

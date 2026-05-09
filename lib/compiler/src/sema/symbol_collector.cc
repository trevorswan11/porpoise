#include "sema/symbol_collector.hh"
#include "ast/nodes.hh"
#include "memory.hh"

namespace porpoise::sema {

auto SymbolCollector::collect_symbols(mod::Module& module, Context& ctx) -> mod::ModuleState {
    if (module.is_collectable()) {
        module.root_table_idx.emplace(ctx.registry.create());

        SymbolCollector collector{module, ctx};
        for (const auto& node : module.tree) { collector.collect(node); }

        if (!ctx.diagnostics.empty()) {
            return module.error_out(std::move(ctx.diagnostics),
                                    mod::ModuleState::POISONED_SYMBOL_COLLECTION);
        }
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
    X(TypeExpression)

#define COLLECTOR_NOOP_X(NodeType) AST_NODE_VISITOR_NOOP(SymbolCollector, NodeType)
MAKE_COLLECTOR_NOOPS(COLLECTOR_NOOP_X)
#undef COLLECTOR_NOOP_X

#define COLLECTOR_NOOP_X(NodeType) AST_TYPE_VISITOR_NOOP(SymbolCollector, NodeType)
FOREACH_AST_TYPE(COLLECTOR_NOOP_X)

auto SymbolCollector::visit(const ast::NodeID&, const ast::ArrayExpression& array) -> void {
    const auto g = in_expr_scope_.guard();
    for (const auto& item : array.items) { collect(*item); }
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::CallExpression& call) -> void {
    const auto g = in_expr_scope_.guard();
    for (const auto& arg : call.arguments) {
        std::visit([&](const auto& handle) { collect(handle); }, arg);
    }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::DoWhileLoopExpression& do_while)
    -> void {
    const auto  g     = loop_guard();
    const auto& block = std::get<ast::BlockStatement>(collecting_.tree[*do_while.block]);
    const auto  idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::EnumExpression& enum_expr) -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::ENUM,
                     IterPair{enum_expr.enumerations,
                              [this](const ast::EnumExpression::Enumeration& enumeration) {
                                  const auto& ident = std::get<ast::IdentifierExpression>(
                                      collecting_.tree[*enumeration.first]);
                                  try_declare(ident.name, enumeration);
                              }},
                     IterPair{enum_expr.members,
                              [this](const ast::Members::Member& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.tree.set_sema_type(id, *last_type_);
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::ForLoopExpression& for_expr) -> void {
    // The guard shouldn't enclose the else clause
    const auto g_expr = in_expr_scope_.guard();
    usize      new_idx;
    {
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto g_loop = in_loop_scope_.guard();
        for (const auto& capture : for_expr.captures) { collect(capture.payload); }
        const auto& block = std::get<ast::BlockStatement>(collecting_.tree[*for_expr.block]);
        for (const auto& stmt : block) { collect(stmt); }
    }
    if (for_expr.non_break) { collect(*for_expr.non_break); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, types::Key::Mutability::IMMUTABLE, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::FunctionExpression& fn) -> void {
    const auto  new_idx = ctx_.registry.create();
    const Scope s{table_stack_, new_idx, table_idx_};
    const auto  g = fn_guard();

    // Parameters belong to the parent scope
    if (fn.self) {
        const auto& ident = std::get<ast::IdentifierExpression>(collecting_.tree[*fn.self->ident]);
        try_declare(ident.name, fn.self.get());
    }

    for (const auto& param : fn.parameters) {
        const auto& ident = std::get<ast::IdentifierExpression>(collecting_.tree[**param.ident]);
        try_declare(ident.name, param);
    }

    if (fn.body) {
        const auto& block = std::get<ast::BlockStatement>(collecting_.tree[**fn.body]);
        for (const auto& stmt : block) { collect(stmt); }
    }

    last_type_.emplace(ctx_.pool[{TypeKind::FUNCTION, types::Key::Mutability::IMMUTABLE, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.tree.set_sema_type(id, *last_type_);
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::IfExpression& if_expr) -> void {
    const auto g = in_expr_scope_.guard();
    collect(if_expr.condition);
    collect(if_expr.consequence);
    if (if_expr.alternate) { collect(*if_expr.alternate); }
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::IndexExpression& index) -> void {
    const auto g = in_expr_scope_.guard();
    collect(index.index);
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::InfiniteLoopExpression& loop)
    -> void {
    const auto  g     = loop_guard();
    const auto& block = std::get<ast::BlockStatement>(collecting_.tree[*loop.block]);
    const auto  idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

#define MAKE_INFIX_COLLECTOR(NodeType)                                              \
    auto SymbolCollector::visit(const ast::NodeID&, const NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                                      \
        collect(node.lhs);                                                          \
        collect(node.rhs);                                                          \
    }

MAKE_INFIX_COLLECTOR(ast::RangeExpression)
MAKE_INFIX_COLLECTOR(ast::AssignmentExpression)
MAKE_INFIX_COLLECTOR(ast::BinaryExpression)

auto SymbolCollector::visit(const ast::NodeID&, const ast::InitializerExpression& init) -> void {
    const auto g = in_expr_scope_.guard();
    for (const auto& initializer : init.initializers) { collect(initializer.value); }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::LabelExpression& label) -> void {
    const auto  g     = label_guard();
    const auto& ident = std::get<ast::IdentifierExpression>(collecting_.tree[*label.name]);
    try_declare(ident.name, SymbolicNode{id});
    collect(label.body);
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::MatchExpression& match) -> void {
    const auto g = in_expr_scope_.guard();
    collect(match.matcher);
    for (const auto& arm : match.arms) {
        const auto  new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        collect(arm.pattern);
        if (arm.capture && (**arm.capture).get_kind() == ast::NodeKind::IDENTIFIER_EXPRESSION) {
            const auto& ident =
                std::get<ast::IdentifierExpression>(collecting_.tree[**arm.capture]);
            try_declare(ident.name, arm);
        }
        collect(arm.dispatch);

        last_type_.emplace(
            ctx_.pool[{TypeKind::MATCH_ARM, types::Key::Mutability::IMMUTABLE, new_idx}]);
        last_type_->set_symbol_table_idx(new_idx);
        collecting_.tree.set_sema_type(arm.pattern, *last_type_.take());
    }
    if (match.catch_all) { collect(*match.catch_all); }
}

#define MAKE_PREFIX_COLLECTOR(NodeType)                                             \
    auto SymbolCollector::visit(const ast::NodeID&, const NodeType& node) -> void { \
        const auto g = in_expr_scope_.guard();                                      \
        collect(node.rhs);                                                          \
    }

MAKE_PREFIX_COLLECTOR(ast::ReferenceExpression)
MAKE_PREFIX_COLLECTOR(ast::DereferenceExpression)
MAKE_PREFIX_COLLECTOR(ast::UnaryExpression)

auto SymbolCollector::visit(const ast::NodeID& id, const ast::StructExpression& struct_expr)
    -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::STRUCT,
                     IterPair{struct_expr.members,
                              [this](const ast::Members::Member& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.tree.set_sema_type(id, *last_type_);
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::UnionExpression& union_expr) -> void {
    const auto scope_idx =
        visit_scopes(TypeKind::UNION,
                     IterPair{union_expr.fields,
                              [this](const ast::UnionExpression::Field& field) {
                                  const auto& ident = std::get<ast::IdentifierExpression>(
                                      collecting_.tree[*field.ident]);
                                  try_declare(ident.name, field);
                              }},
                     IterPair{union_expr.members,
                              [this](const ast::Members::Member& member) { collect(*member); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.tree.set_sema_type(id, *last_type_);
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::WhileLoopExpression& while_expr)
    -> void {
    // The guard shouldn't enclose the else clause
    const auto g_expr = in_expr_scope_.guard();
    usize      new_idx;
    {
        new_idx = ctx_.registry.create();
        const Scope s{table_stack_, new_idx, table_idx_};

        const auto  g_loop = in_loop_scope_.guard();
        const auto& block  = std::get<ast::BlockStatement>(collecting_.tree[*while_expr.block]);
        for (const auto& stmt : block) { collect(stmt); }
    }
    if (while_expr.non_break) { collect(*while_expr.non_break); }

    last_type_.emplace(ctx_.pool[{TypeKind::BLOCK, types::Key::Mutability::IMMUTABLE, new_idx}]);
    last_type_->set_symbol_table_idx(new_idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::BlockStatement& block) -> void {
    if (!in_expr_scope_ && table_stack_.size() == 1) {
        ctx_.diagnostics.emplace_back("Cannot have block at the top level",
                                      Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                      collecting_.tree.location_of(id));
    }

    const auto scope_idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::BreakStatement& break_stmt) -> void {
    if (!in_loop_scope_ && !in_label_scope_) {
        ctx_.diagnostics.emplace_back("Cannot break outside of a loop or label",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.tree.location_of(id));
    }
    if (break_stmt.expression) { collect(*break_stmt.expression); }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::ContinueStatement&) -> void {
    if (!in_loop_scope_) {
        ctx_.diagnostics.emplace_back("Cannot continue outside of a loop",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.tree.location_of(id));
    }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::DeclStatement& decl) -> void {
    // We can stop analyzing early if there's no value
    const auto& ident = std::get<ast::IdentifierExpression>(collecting_.tree[*decl.ident]);
    const auto  name  = ident.name;
    if (!try_declare(name, SymbolicNode{id})) { return; };
    if (!decl.value) { return; }

    // Valued decls should be evaluated to get shallow types
    auto&      symbol = ctx_.registry.get_from(table_idx_, name);
    const auto expr   = *decl.value;
    if (expr->any<ast::EnumExpression, ast::UnionExpression, ast::StructExpression>()) {
        if (decl.has_modifier(ast::DeclModifiers::CONSTEXPR)) {
            ctx_.diagnostics.emplace_back(
                fmt::format("All {}s are implicitly constexpr", expr->display_name()),
                Error::REDUNDANT_CONSTEXPR,
                collecting_.tree.location_of(id));
        } else if (!decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            ctx_.poison_symbol(symbol,
                               fmt::format("All {}s must be marked const", expr->display_name()),
                               Error::ILLEGAL_NON_CONST_STATEMENT,
                               collecting_.tree.location_of(id));
        } else {
            symbol.set_kind(SymbolKind::TYPE);
        }
    } else if (expr->is<ast::FunctionExpression>()) {
        if (!decl.has_modifier(ast::DeclModifiers::CONSTEXPR) &&
            !decl.has_modifier(ast::DeclModifiers::CONSTANT)) {
            ctx_.poison_symbol(symbol,
                               "All function declarations must be const or constexpr",
                               Error::ILLEGAL_NON_CONST_STATEMENT,
                               collecting_.tree.location_of(id));
        } else {
            symbol.set_kind(SymbolKind::CALLABLE);
        }
    }

    collect(expr);
    if (last_type_) { ctx_.registry.get_from(table_idx_, name).set_type(*last_type_.take()); }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::DeferStatement& defer) -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot have defer outside of a function's scope",
                                      Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                      collecting_.tree.location_of(id));
    }
    collect(defer.deferred);
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::DiscardStatement& discard) -> void {
    collect(discard.discarded);
}

auto SymbolCollector::visit(const ast::NodeID&, const ast::ExpressionStatement& expr) -> void {
    collect(expr.expression);
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::ImportStatement& import_stmt)
    -> void {
    auto [alias, mod_result] =
        [&] -> std::pair<std::string_view, Result<mem::NonNull<mod::Module>, mod::Diagnostic>> {
        if (import_stmt.payload->is<ast::StringExpression>()) {
            ASSERT(import_stmt.alias, "File import without alias");
            const auto& ident =
                std::get<ast::IdentifierExpression>(collecting_.tree[**import_stmt.alias]);
            const auto& string =
                std::get<ast::StringExpression>(collecting_.tree[*import_stmt.payload]);

            return {ident.name,
                    ctx_.modules.try_get_file_module(string.value, collecting_.parent_path)};
        }

        const auto& ident =
            import_stmt.alias
                ? std::get<ast::IdentifierExpression>(collecting_.tree[**import_stmt.alias])
                : std::get<ast::IdentifierExpression>(collecting_.tree[*import_stmt.payload]);
        return {ident.name, ctx_.modules.try_get_library_module(std::string{ident.name})};
    }();

    // Only set the table index if the module exists
    opt::Option<mod::Module&> imported_mod;
    if (!mod_result) {
        // The token is retrieved here to avoid copying it on success
        ctx_.diagnostics.emplace_back(mod_result.error().get_message(),
                                      Error::MODULE_LOAD_ERROR,
                                      collecting_.tree.location_of(*import_stmt.payload));
    } else {
        imported_mod.emplace(**mod_result);
        Context new_ctx = ctx_;
        collect_symbols(*imported_mod, new_ctx);
    }

    ctx_.try_result(ctx_.registry.insert_into<SymbolicImport>(
        table_idx_, collecting_, alias, ast::ImportHandle{id}, imported_mod));
    if (imported_mod) {
        auto& type = ctx_.pool[{
            TypeKind::MODULE, types::Key::Mutability::IMMUTABLE, *imported_mod->root_table_idx}];
        collecting_.tree.set_sema_type(id, type);
        auto& symbol = ctx_.registry.get_from(table_idx_, alias);
        symbol.set_type(type);
        symbol.set_kind(SymbolKind::MODULE);
    } else {
        ctx_.poison_symbol(ctx_.registry.get_from(table_idx_, alias));
    }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::ReturnStatement& return_stmt)
    -> void {
    if (!in_function_scope_) {
        ctx_.diagnostics.emplace_back("Cannot return outside of a function",
                                      Error::ILLEGAL_CONTROL_FLOW,
                                      collecting_.tree.location_of(id));
    }
    if (return_stmt.expression) { collect(*return_stmt.expression); }
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::TestStatement& test) -> void {
    if (table_stack_.size() != 1) {
        ctx_.diagnostics.emplace_back("Tests must be at the topmost level of a file",
                                      Error::ILLEGAL_TEST_LOCATION,
                                      collecting_.tree.location_of(id));
    }

    // Not a symbol so don't push to the table, track in node instead
    const auto& block = std::get<ast::BlockStatement>(collecting_.tree[*test.block]);
    const auto  scope_idx =
        visit_scopes(TypeKind::BLOCK,
                     IterPair{block, [this](const ast::StatementHandle& stmt) { collect(stmt); }});
    last_type_->set_symbol_table_idx(scope_idx);
    collecting_.tree.set_sema_type(id, *last_type_.take());
}

auto SymbolCollector::visit(const ast::NodeID& id, const ast::UsingStatement& using_stmt) -> void {
    const auto& ident = std::get<ast::IdentifierExpression>(collecting_.tree[*using_stmt.alias]);
    try_declare(ident.name, SymbolicNode{id});
    ctx_.registry.get_from(table_idx_, ident.name).set_kind(SymbolKind::TYPE);
}

auto SymbolCollector::visit(const ast::NodeID&, const Unit&) -> void {}

} // namespace porpoise::sema

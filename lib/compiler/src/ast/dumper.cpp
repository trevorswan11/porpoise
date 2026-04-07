#include <fmt/format.h>
#include <fmt/ostream.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

namespace porpoise::ast {

auto ASTDumper::visit(const ArrayExpression& array) -> void {
    fmt::println(out_, "ArrayExpression");
    {
        const Indent::Guard g{indent_, false};
        if (array.has_explicit_size()) {
            fmt::print(out_, "{}Size: ", indent_.current_branch());
            array.get_explicit_size().accept(*this);
        } else {
            fmt::println(out_, "{}Size: (inferred)", indent_.current_branch());
        }
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        visit(array.get_item_type());
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Items:", indent_.current_branch());
        dump_node_list(array.get_items());
    }
}

auto ASTDumper::visit(const CallArgument& argument) -> void {
    if (argument.is_expression()) {
        argument.get_expression().accept(*this);
    } else {
        visit(argument.get_type());
    }
}

auto ASTDumper::visit(const CallExpression& call) -> void {
    fmt::println(out_, "CallExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Callee: ", indent_.current_branch());
        call.get_function().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Arguments:", indent_.current_branch());
        dump_node_list(call.get_arguments());
    }
}

auto ASTDumper::visit(const DoWhileLoopExpression& do_while) -> void {
    fmt::println(out_, "DoWhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        do_while.get_block().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        do_while.get_condition().accept(*this);
    }
}

auto ASTDumper::visit(const Enumeration& enumeration) -> void {
    {
        fmt::print(out_, "Name: ");
        const Indent::Guard g_name{indent_, !enumeration.has_default_value()};
        enumeration.get_ident().accept(*this);
    }

    if (enumeration.has_default_value()) {
        const Indent::Guard g_val{indent_, true};
        fmt::print(out_, "{}Default: ", indent_.current_branch());
        enumeration.get_default_value().accept(*this);
    }
}

auto ASTDumper::visit(const EnumExpression& enum_expr) -> void {
    fmt::println(out_, "EnumExpression");

    if (enum_expr.has_underlying()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Underlying: ", indent_.current_branch());
        enum_expr.get_underlying().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Enumerations:", indent_.current_branch());
        dump_node_list(enum_expr.get_enumerations());
    }
}

auto ASTDumper::visit(const ForLoopCapture& capture) -> void {
    if (capture.is_discarded()) {
        fmt::println(out_, "<discarded>");
    } else {
        const auto& valued = capture.get_valued();
        fmt::println(out_, "{} (modifier: {})", valued.get_ident(), valued.get_modifier());
    }
}

auto ASTDumper::visit(const ForLoopExpression& for_loop) -> void {
    fmt::println(out_, "ForLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Iterables:", indent_.current_branch());
        dump_node_list(for_loop.get_iterables());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Captures:", indent_.current_branch());
        dump_node_list(for_loop.get_captures());
    }

    {
        const Indent::Guard g{indent_, !for_loop.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        for_loop.get_block().accept(*this);
    }

    if (for_loop.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break: ", indent_.current_branch());
        for_loop.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const SelfParameter& self) -> void {
    fmt::println(out_, "Self: {} (modifier: {})", self.get_ident(), self.get_modifier());
}

auto ASTDumper::visit(const FunctionParameter& parameter) -> void {
    fmt::println(out_, "Param:");
    if (parameter.has_ident()) {
        const Indent::Guard g_name{indent_, false};
        fmt::print(out_, "{}Name: ", indent_.current_branch());
        parameter.get_ident().accept(*this);
    }
    {
        const Indent::Guard g_type{indent_, true};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        visit(parameter.get_type());
    }
}

auto ASTDumper::visit(const FunctionExpression& function) -> void {
    fmt::println(out_, "FunctionExpression");
    if (function.has_self()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}", indent_.current_branch());
        visit(function.get_self());
    }

    if (function.has_parameters()) {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Parameters:", indent_.current_branch());
        dump_node_list(function.get_parameters());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Variadic: {}", indent_.current_branch(), function.is_variadic());
    }

    {
        const Indent::Guard g{indent_, !function.has_body()};
        fmt::print(out_, "{}Returns: ", indent_.current_branch());
        visit(function.get_return_type());
    }

    if (function.has_body()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        function.get_body().accept(*this);
    }
}

auto ASTDumper::visit(const IdentifierExpression& ident) -> void {
    fmt::print(out_, "IdentifierExpression: {}", ident);
    if (ident.get_token().is_builtin()) {
        fmt::print(out_, " (builtin)");
    } else if (ident.get_token().is_primitive()) {
        fmt::print(out_, " (primitive)");
    }
    fmt::println(out_, "");
}

auto ASTDumper::visit(const IfExpression& if_expr) -> void {
    fmt::println(out_, "IfExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Constexpr: {}", indent_.current_branch(), if_expr.is_constexpr());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        if_expr.get_condition().accept(*this);
    }

    {
        Indent::Guard g{indent_, !if_expr.has_alternate()};
        fmt::print(out_, "{}Consequence: ", indent_.current_branch());
        if_expr.get_consequence().accept(*this);
    }

    if (if_expr.has_alternate()) {
        Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alternate: ", indent_.current_branch());
        if_expr.get_alternate().accept(*this);
    }
}

auto ASTDumper::visit(const IndexExpression& index) -> void {
    fmt::println(out_, "IndexExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Object: ", indent_.current_branch());
        index.get_array().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Index: ", indent_.current_branch());
        index.get_index().accept(*this);
    }
}

auto ASTDumper::visit(const InfiniteLoopExpression& loop) -> void {
    fmt::println(out_, "InfiniteLoopExpression");
    dump_node_list(loop.get_block());
}

#define MAKE_INFIX_DUMP(NodeType, LeftLabel, RightLabel)                             \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        {                                                                            \
            const Indent::Guard g{indent_, false};                                   \
            fmt::print(out_, "{}" #LeftLabel ": ", indent_.current_branch());        \
            node.get_lhs().accept(*this);                                            \
        }                                                                            \
        {                                                                            \
            const Indent::Guard g{indent_, true};                                    \
            fmt::print(out_, "{}" #RightLabel ": ", indent_.current_branch());       \
            node.get_rhs().accept(*this);                                            \
        }                                                                            \
    }

MAKE_INFIX_DUMP(AssignmentExpression, Assignee, Value)
MAKE_INFIX_DUMP(BinaryExpression, LHS, RHS)
MAKE_INFIX_DUMP(DotExpression, Object, Member)
MAKE_INFIX_DUMP(RangeExpression, Lower, Upper)

auto ASTDumper::visit(const Initializer& init) -> void { TODO(init); }
auto ASTDumper::visit(const InitializerExpression& init) -> void { TODO(init); }

auto ASTDumper::visit(const MatchArm& arm) -> void {
    fmt::println(out_, "Arm:");
    {
        const Indent::Guard g_pattern{indent_, false};
        fmt::print(out_, "{}Pattern: ", indent_.current_branch());
        arm.get_pattern().accept(*this);
    }

    if (arm.has_capture_clause()) {
        const Indent::Guard g_pattern{indent_, false};
        fmt::print(out_, "{}Capture: ", indent_.current_branch());
        if (arm.is_explicit_capture()) {
            arm.get_explicit_capture().accept(*this);
        } else {
            fmt::println(out_, "<discarded>");
        }
    }

    {
        const Indent::Guard g_result{indent_, true};
        fmt::print(out_, "{}Dispatch: ", indent_.current_branch());
        arm.get_dispatch().accept(*this);
    }
}

auto ASTDumper::visit(const MatchExpression& match) -> void {
    fmt::println(out_, "MatchExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Matcher: ", indent_.current_branch());
        match.get_matcher().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !match.has_catch_all()};
        fmt::println(out_, "{}Arms:", indent_.current_branch());
        dump_node_list(match.get_arms());
    }

    if (match.has_catch_all()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Catch All: ", indent_.current_branch());
        match.get_catch_all().accept(*this);
    }
}

#define MAKE_PREFIX_DUMP(NodeType)                                                   \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        const Indent::Guard g{indent_, true};                                        \
        fmt::print(out_, "{}Operand: ", indent_.current_branch());                   \
        node.get_rhs().accept(*this);                                                \
    }

MAKE_PREFIX_DUMP(ReferenceExpression)
MAKE_PREFIX_DUMP(DereferenceExpression)
MAKE_PREFIX_DUMP(UnaryExpression)
MAKE_PREFIX_DUMP(ImplicitAccessExpression)

#define MAKE_LEAF_DUMP(NodeType)                          \
    auto ASTDumper::visit(const NodeType& node) -> void { \
        fmt::println(out_, #NodeType ": {}", node);       \
    }

MAKE_LEAF_DUMP(StringExpression)
MAKE_LEAF_DUMP(I32Expression)
MAKE_LEAF_DUMP(I64Expression)
MAKE_LEAF_DUMP(ISizeIntegerExpression)
MAKE_LEAF_DUMP(U32Expression)
MAKE_LEAF_DUMP(U64Expression)
MAKE_LEAF_DUMP(USizeIntegerExpression)
MAKE_LEAF_DUMP(U8Expression)
MAKE_LEAF_DUMP(F32Expression)
MAKE_LEAF_DUMP(F64Expression)
MAKE_LEAF_DUMP(BoolExpression)

auto ASTDumper::visit(const ScopeResolutionExpression& scope_resolve) -> void {
    fmt::println(out_, "ScopeResolutionExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Outer: ", indent_.current_branch());
        scope_resolve.get_outer().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Inner: ", indent_.current_branch());
        scope_resolve.get_inner().accept(*this);
    }
}

auto ASTDumper::visit(const StructExpression& struct_expr) -> void {
    fmt::println(out_, "StructExpression{}", struct_expr.is_packed() ? " (packed)" : "");
    dump_node_list(struct_expr.get_members());
}

auto ASTDumper::visit(const ExplicitType& type) -> void {
    fmt::println(out_, "ExplicitType (modifier: {})", type.get_modifier());

    const Indent::Guard g{indent_, true};
    type.match(Overloaded{
        [this](const auto& t) {
            fmt::print(out_, "{}", indent_.current_branch());
            t->accept(*this);
        },
        [this](const ExplicitArrayType& a) {
            fmt::println(out_, "{}ArrayType", indent_.current_branch());
            {
                const Indent::Guard g_inner{indent_, false};
                fmt::print(out_, "{}Dimensions: ", indent_.current_branch());
                if (a.has_dimension()) {
                    a.get_dimension().accept(*this);
                } else {
                    fmt::println(out_, "(slice)");
                }
            }

            {
                const Indent::Guard g_inner{indent_, false};
                fmt::println(out_,
                             "{}Null terminated: {}",
                             indent_.current_branch(),
                             a.is_null_terminated());
            }

            {
                const Indent::Guard g_inner{indent_, true};
                fmt::print(out_, "{}", indent_.current_branch());
                visit(a.get_inner_type());
            }
        },
        [this](const ExplicitType::ExplicitRecursiveType& r) {
            fmt::print(out_, "{}", indent_.current_branch());
            visit(*r);
        },
    });
}

auto ASTDumper::visit(const TypeExpression& node) -> void {
    if (node.has_explicit_type()) {
        visit(node.get_explicit_type());
    } else {
        fmt::println(out_, "(inferred)");
    }
}

auto ASTDumper::visit(const UnionField& field) -> void {
    fmt::println(out_, "Field:");
    {
        const Indent::Guard g_pattern{indent_, false};
        fmt::print(out_, "{}Tag: ", indent_.current_branch());
        field.get_ident().accept(*this);
    }

    {
        const Indent::Guard g_result{indent_, true};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        visit(field.get_type());
    }
}

auto ASTDumper::visit(const UnionExpression& union_expr) -> void {
    fmt::println(out_, "UnionExpression");
    dump_node_list(union_expr.get_fields());
}

auto ASTDumper::visit(const WhileLoopExpression& while_expr) -> void {
    fmt::println(out_, "WhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        while_expr.get_condition().accept(*this);
    }

    if (while_expr.has_continuation()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Continuation: ", indent_.current_branch());
        while_expr.get_continuation().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !while_expr.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        while_expr.get_block().accept(*this);
    }

    if (while_expr.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break Clause: ", indent_.current_branch());
        while_expr.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const BlockStatement& block) -> void {
    fmt::println(out_, "BlockStatement");
    if (block.empty()) {
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}<empty>", indent_.current_branch());
    } else {
        dump_node_list(block);
    }
}

auto ASTDumper::visit(const DeclStatement& decl) -> void {
    fmt::println(out_, "DeclStatement ({})", decl.get_ident());
    {
        const Indent::Guard g{indent_, false};
        const auto          flags = magic_enum::enum_flags_name(decl.get_modifiers());
        fmt::println(out_, "{}Modifiers: {}", indent_.current_branch(), flags);
    }

    {
        const Indent::Guard g{indent_, !decl.has_value()};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        decl.get_type().accept(*this);
    }

    if (decl.has_value()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        decl.get_value().accept(*this);
    }
}

#define MAKE_BASIC_STMT_DUMP(NodeType, FieldName, getter)                     \
    auto ASTDumper::visit(const NodeType& node) -> void {                     \
        fmt::println(out_, #NodeType);                                        \
        {                                                                     \
            const Indent::Guard g{indent_, true};                             \
            fmt::print(out_, "{}" #FieldName ": ", indent_.current_branch()); \
            node.getter.accept(*this);                                        \
        }                                                                     \
    }

MAKE_BASIC_STMT_DUMP(DeferStatement, Deferred, get_deferred())
MAKE_BASIC_STMT_DUMP(DiscardStatement, Discarded, get_discarded())
MAKE_BASIC_STMT_DUMP(ExpressionStatement, Expr, get_expression())

auto ASTDumper::visit(const ImportStatement& import_stmt) -> void {
    fmt::println(out_, "ImportStatement");
    {
        const Indent::Guard g{indent_, !import_stmt.has_alias()};
        if (import_stmt.is_library_import()) {
            fmt::print(out_, "{}Library: ", indent_.current_branch());
            import_stmt.get_library_import().get_name().accept(*this);
        } else {
            fmt::print(out_, "{}File: ", indent_.current_branch());
            import_stmt.get_file_import().get_file().accept(*this);
        }
    }

    if (import_stmt.has_alias()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        if (import_stmt.is_library_import()) {
            import_stmt.get_library_import().get_alias().accept(*this);
        } else {
            import_stmt.get_file_import().get_alias().accept(*this);
        }
    }
}

auto ASTDumper::visit(const JumpStatement& jump) -> void {
    fmt::println(out_, "JumpStatement ({})", magic_enum::enum_name(jump.get_token().type));
    if (jump.has_expression()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        jump.get_expression().accept(*this);
    }
}

auto ASTDumper::visit(const ModuleStatement&) -> void { fmt::println(out_, "ModuleStatement"); }

auto ASTDumper::visit(const UsingStatement& using_stmt) -> void {
    fmt::println(out_, "UsingStatement");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        using_stmt.get_alias().accept(*this);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        visit(using_stmt.get_type());
    }
}

} // namespace porpoise::ast

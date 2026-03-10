#include <fmt/format.h>
#include <fmt/ostream.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/ast.hpp"
#include "ast/dumper.hpp"

namespace conch::ast {

#define MAKE_INFIX_DUMP(NodeType)                                                    \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        {                                                                            \
            const Indent::Guard g{indent_, false};                                   \
            fmt::print(out_, "{}", indent_.current_branch());                        \
            node.get_lhs().accept(*this);                                            \
        }                                                                            \
        {                                                                            \
            const Indent::Guard g{indent_, true};                                    \
            fmt::print(out_, "{}", indent_.current_branch());                        \
            node.get_rhs().accept(*this);                                            \
        }                                                                            \
    }

#define MAKE_PREFIX_DUMP(NodeType)                                                   \
    auto ASTDumper::visit(const NodeType& node) -> void {                            \
        fmt::println(out_, #NodeType " ({})", magic_enum::enum_name(node.get_op())); \
        const Indent::Guard g{indent_, true};                                        \
        fmt::print(out_, "{}", indent_.current_branch());                            \
        node.get_rhs().accept(*this);                                                \
    }

#define MAKE_LEAF_DUMP(NodeType, getter)                   \
    auto ASTDumper::visit(const NodeType& node) -> void {  \
        fmt::println(out_, #NodeType ": {}", node.getter); \
    }

auto ASTDumper::visit(const ArrayExpression& node) -> void {
    fmt::println(out_, "ArrayExpression");
    {
        const Indent::Guard g{indent_, false};
        if (node.has_explicit_size()) {
            fmt::print(out_, "{}Size: ", indent_.current_branch());
            node.get_explicit_size().accept(*this);
        } else {
            fmt::print(out_, "{}Size: (inferred)", indent_.current_branch());
        }
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        dump_explicit_type(node.get_item_type(), false);
    }

    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Items: ", indent_.current_branch());
        dump_list(node.get_items());
    }
}

auto ASTDumper::visit(const CallExpression& node) -> void {
    fmt::println(out_, "CallExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Callee: ", indent_.current_branch());
        node.get_function().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Arguments: ", indent_.current_branch());
        dump_list(node.get_arguments());
    }
}

auto ASTDumper::visit(const DoWhileLoopExpression& node) -> void {
    fmt::println(out_, "DoWhileLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }
}

auto ASTDumper::visit(const EnumExpression& node) -> void {
    fmt::println(out_, "EnumExpression");

    if (node.has_underlying()) {
        const Indent::Guard g{indent_, node.get_enumerations().empty()};
        fmt::print(out_, "{}Name: ", indent_.current_branch());
        node.get_underlying().accept(*this);
    }

    {
        auto                variants = node.get_enumerations();
        const Indent::Guard g{indent_, true};
        fmt::println(out_, "{}Variants:", indent_.current_branch());

        for (auto it = variants.begin(); it != variants.end(); ++it) {
            const Indent::Guard g_var{indent_, std::next(it) == variants.end()};
            fmt::print(out_, "{}{}", indent_.current_branch(), it->get_enumeration().get_name());
            if (it->has_default_value()) {
                fmt::print(out_, " = ");
                it->get_default_value().accept(*this);
            } else {
                out_ << "\n";
            }
        }
    }
}

auto ASTDumper::visit(const ForLoopExpression& node) -> void {
    fmt::println(out_, "ForLoopExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Iterables:", indent_.current_branch());
        dump_list(node.get_iterables());
    }

    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Captures:", indent_.current_branch());
        auto captures = node.get_captures();
        for (size_t i = 0; i < captures.size(); ++i) {
            const Indent::Guard g_cap{indent_, i == captures.size() - 1};
            if (captures[i].is_discarded()) {
                fmt::println(out_, "{}_ (discarded)", indent_.current_branch());
            } else {
                const auto& valued = captures[i].get_valued();
                fmt::println(out_,
                             "{}{} (modifier: {})",
                             indent_.current_branch(),
                             valued.get_name().get_name(),
                             valued.get_modifier());
            }
        }
    }

    {
        const Indent::Guard g{indent_, !node.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }

    if (node.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break Clause: ", indent_.current_branch());
        node.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const FunctionExpression& node) -> void {
    fmt::println(out_, "FunctionExpression");
    if (node.has_self()) {
        const Indent::Guard g{indent_, false};
        const auto&         self = node.get_self();
        fmt::println(out_,
                     "{}Self: {} (modifier: {})",
                     indent_.current_branch(),
                     self.get_name().get_name(),
                     self.get_modifier());
    }

    auto params = node.get_parameters();
    {
        const Indent::Guard g{indent_, false};
        fmt::println(out_, "{}Parameters:", indent_.current_branch());
        for (auto it = params.begin(); it != params.end(); ++it) {
            const Indent::Guard g_param{indent_, std::next(it) == params.end()};
            fmt::println(out_, "{}Param: ", indent_.current_branch());
            {
                const Indent::Guard gi{indent_, false};
                fmt::println(
                    out_, "{}Name: {}", indent_.current_branch(), it->get_name().get_name());
            }
            {
                const Indent::Guard gi{indent_, true};
                fmt::print(out_, "{}Type: ", indent_.current_branch());
                dump_explicit_type(it->get_type(), false);
            }
        }
    }

    {
        const Indent::Guard g{indent_, !node.has_body()};
        fmt::print(out_, "{}Returns: ", indent_.current_branch());
        dump_explicit_type(node.get_return_type(), false);
    }

    if (node.has_body()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_body().accept(*this);
    }
}

MAKE_LEAF_DUMP(IdentifierExpression, get_name())

auto ASTDumper::visit(const IfExpression& node) -> void {
    fmt::println(out_, "IfExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }

    {
        Indent::Guard g{indent_, !node.has_alternate()};
        fmt::print(out_, "{}Then:", indent_.current_branch());
        node.get_consequence().accept(*this);
    }

    if (node.has_alternate()) {
        Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Else:", indent_.current_branch());
        node.get_alternate().accept(*this);
    }
}

auto ASTDumper::visit(const IndexExpression& node) -> void {
    fmt::println(out_, "IndexExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Object: ", indent_.current_branch());
        node.get_array().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Index: ", indent_.current_branch());
        node.get_index().accept(*this);
    }
}

auto ASTDumper::visit(const InfiniteLoopExpression& node) -> void {
    fmt::println(out_, "InfiniteLoopExpression");
    dump_list(node.get_block());
}

MAKE_INFIX_DUMP(AssignmentExpression)
MAKE_INFIX_DUMP(BinaryExpression)
MAKE_INFIX_DUMP(DotExpression)
MAKE_INFIX_DUMP(RangeExpression)

auto ASTDumper::visit(const MatchExpression& node) -> void {
    fmt::println(out_, "MatchExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Input: ", indent_.current_branch());
        node.get_matcher().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !node.has_catch_all()};
        fmt::println(out_, "{}Arms:", indent_.current_branch());
        auto arms = node.get_arms();
        for (auto it = arms.begin(); it != arms.end(); ++it) {
            const Indent::Guard g_arm{indent_, std::next(it) == arms.end()};
            fmt::println(out_, "{}Arm", indent_.current_branch());
            {
                const Indent::Guard g_pattern{indent_, false};
                fmt::print(out_, "{}Pattern: ", indent_.current_branch());
                it->get_pattern().accept(*this);
            }
            {
                const Indent::Guard g_result{indent_, true};
                fmt::print(out_, "{}Dispatch: ", indent_.current_branch());
                it->get_dispatch().accept(*this);
            }
        }
    }

    if (node.has_catch_all()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Catch All: ", indent_.current_branch());
        node.get_catch_all().accept(*this);
    }
}

MAKE_PREFIX_DUMP(ReferenceExpression)
MAKE_PREFIX_DUMP(DereferenceExpression)
MAKE_PREFIX_DUMP(UnaryExpression)

MAKE_LEAF_DUMP(StringExpression, get_value())
MAKE_LEAF_DUMP(SignedIntegerExpression, get_value())
MAKE_LEAF_DUMP(SignedLongIntegerExpression, get_value())
MAKE_LEAF_DUMP(ISizeIntegerExpression, get_value())
MAKE_LEAF_DUMP(UnsignedIntegerExpression, get_value())
MAKE_LEAF_DUMP(UnsignedLongIntegerExpression, get_value())
MAKE_LEAF_DUMP(USizeIntegerExpression, get_value())
MAKE_LEAF_DUMP(ByteExpression, get_value())
MAKE_LEAF_DUMP(FloatExpression, get_value())
MAKE_LEAF_DUMP(BoolExpression, get_value())

auto ASTDumper::visit(const ScopeResolutionExpression& node) -> void {
    fmt::println(out_, "ScopeResolutionExpression");
    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Outer: ", indent_.current_branch());
        node.get_outer().accept(*this);
    }
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Inner: ", indent_.current_branch());
        node.get_inner().accept(*this);
    }
}

auto ASTDumper::visit(const StructExpression& node) -> void {
    fmt::println(out_, "StructExpression{}", node.is_packed() ? " (packed)" : "");
    dump_list(node.get_members());
}

auto ASTDumper::visit(const TypeExpression& node) -> void {
    if (node.has_explicit_type()) {
        dump_explicit_type(node.get_explicit_type(), false);
    } else {
        fmt::println(out_, "(inferred)");
    }
}

auto ASTDumper::visit(const WhileLoopExpression& node) -> void {
    fmt::println(out_, "WhileLoopExpression");

    {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Condition: ", indent_.current_branch());
        node.get_condition().accept(*this);
    }

    if (node.has_continuation()) {
        const Indent::Guard g{indent_, false};
        fmt::print(out_, "{}Continuation: ", indent_.current_branch());
        node.get_continuation().accept(*this);
    }

    {
        const Indent::Guard g{indent_, !node.has_non_break()};
        fmt::print(out_, "{}Body: ", indent_.current_branch());
        node.get_block().accept(*this);
    }

    if (node.has_non_break()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Non-Break Clause: ", indent_.current_branch());
        node.get_non_break().accept(*this);
    }
}

auto ASTDumper::visit(const BlockStatement& node) -> void {
    fmt::println(out_, "BlockStatement");
    dump_list(node);
}

auto ASTDumper::visit(const DeclStatement& node) -> void {
    fmt::println(out_, "DeclStatement ({})", node.get_ident().get_name());

    {
        const Indent::Guard g{indent_, !node.has_value()};
        const auto          flags = magic_enum::enum_flags_name(node.get_modifiers());
        fmt::println(out_, "{}Modifiers: {}", indent_.current_branch(), flags);
    }

    {
        const Indent::Guard g{indent_, !node.has_value()};
        fmt::print(out_, "{}Type: ", indent_.current_branch());
        node.get_type().accept(*this);
    }

    if (node.has_value()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        node.get_value().accept(*this);
    }
}

auto ASTDumper::visit(const DiscardStatement& node) -> void {
    fmt::println(out_, "DiscardStatement");
    {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Discarded: ", indent_.current_branch());
        node.get_discarded().accept(*this);
    }
}

auto ASTDumper::visit(const ExpressionStatement& node) -> void {
    node.get_expression().accept(*this);
}

auto ASTDumper::visit(const ImportStatement& node) -> void {
    fmt::println(out_, "ImportStatement");
    {
        const Indent::Guard g{indent_, !node.has_alias()};
        if (node.is_module_import()) {
            fmt::print(out_, "{}Module: ", indent_.current_branch());
            node.get_module_import().accept(*this);
        } else {
            fmt::print(out_, "{}User: ", indent_.current_branch());
            node.get_user_import().accept(*this);
        }
    }

    if (node.has_alias()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Alias: ", indent_.current_branch());
        node.get_alias().accept(*this);
    }
}

auto ASTDumper::visit(const JumpStatement& node) -> void {
    fmt::println(out_, "JumpStatement ({})", magic_enum::enum_name(node.get_token().type));
    if (node.has_expression()) {
        const Indent::Guard g{indent_, true};
        fmt::print(out_, "{}Value: ", indent_.current_branch());
        node.get_expression().accept(*this);
    }
}

auto ASTDumper::dump_explicit_type(const ExplicitType& type, bool print_branch) -> void {
    if (print_branch) { fmt::print(out_, "{}", indent_.current_branch()); }
    fmt::println(out_, "ExplicitType (modifier: {})", type.get_modifier());

    const Indent::Guard g{indent_, true};
    std::visit(
        Overloaded{
            [this](const ExplicitType::ExplicitIdentType& t) {
                fmt::print(out_, "{}", indent_.current_branch());
                t->accept(*this);
            },
            [this](const ExplicitType::ExplicitFunctionType& f) {
                fmt::print(out_, "{}", indent_.current_branch());
                f->accept(*this);
            },
            [this](const ExplicitArrayType& a) {
                fmt::println(out_, "{}ArrayType", indent_.current_branch());
                {
                    const Indent::Guard g_inner{indent_, false};
                    fmt::print(out_, "{}Dimensions: ", indent_.current_branch());
                    a.get_dimensions().accept(*this);
                }
                {
                    const Indent::Guard g_inner{indent_, true};
                    dump_explicit_type(a.get_inner_type(), true);
                }
            },
            [this](const ExplicitType::ExplicitRecursiveType& r) { dump_explicit_type(*r, true); },
        },
        type.get_type());
}

} // namespace conch::ast

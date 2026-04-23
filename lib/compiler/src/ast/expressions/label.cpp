#include "ast/expressions/label.hpp"

#include "ast/expressions/do_while.hpp"
#include "ast/expressions/for.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/if.hpp"
#include "ast/expressions/infinite_loop.hpp"
#include "ast/expressions/match.hpp"
#include "ast/expressions/while.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/expression.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

LabelExpression::LabelExpression(const syntax::Token&           start_token,
                                 mem::Box<IdentifierExpression> name,
                                 LabeledNode                    body) noexcept
    : ExprBase{start_token}, name_{std::move(name)}, body_{std::move(body)} {}
LabelExpression::~LabelExpression() = default;

auto LabelExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto LabelExpression::parse(syntax::Parser& parser, mem::Box<Expression> name)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto& start_token = name->get_token();
    if (!name->is<IdentifierExpression>()) {
        return make_parser_err(syntax::ParserError::ILLEGAL_LABEL, start_token);
    }
    parser.advance();

    // The body has to be constructed in place from a lambda as it cannot be default initialized
    auto raw_stmt = TRY(parser.parse_statement(true));
    auto body     = TRY(deconstruct_body(std::move(raw_stmt)));

    return mem::make_box<LabelExpression>(
        start_token, downcast<IdentifierExpression>(std::move(name)), std::move(body));
}

auto LabelExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted   = as<LabelExpression>(other);
    const auto  names_eq = *name_ == *casted.name_;
    if (!names_eq || body_.index() != casted.body_.index()) { return false; }

    const auto& other_body = casted.body_;
    return match([&other_body](const auto& b) {
        return *b == *std::get<std::remove_cvref_t<decltype(b)>>(other_body);
    });
}

auto LabelExpression::deconstruct_body(mem::Box<ast::Statement>&& raw_stmt)
    -> Result<LabeledNode, syntax::ParserDiagnostic> {
    switch (raw_stmt->get_kind()) {
    case NodeKind::EXPRESSION_STATEMENT: {
        auto expr_stmt = Node::downcast<ExpressionStatement>(std::move(raw_stmt));
        switch (expr_stmt->get_expression().get_kind()) {
        case NodeKind::DO_WHILE_LOOP_EXPRESSION:
            return Node::downcast<DoWhileLoopExpression>(std::move(expr_stmt->expression_));
        case NodeKind::FOR_LOOP_EXPRESSION:
            return Node::downcast<ForLoopExpression>(std::move(expr_stmt->expression_));
        case NodeKind::IF_EXPRESSION:
            return Node::downcast<IfExpression>(std::move(expr_stmt->expression_));
        case NodeKind::INFINITE_LOOP_EXPRESSION:
            return Node::downcast<InfiniteLoopExpression>(std::move(expr_stmt->expression_));
        case NodeKind::MATCH_EXPRESSION:
            return Node::downcast<MatchExpression>(std::move(expr_stmt->expression_));
        case NodeKind::WHILE_LOOP_EXPRESSION:
            return Node::downcast<WhileLoopExpression>(std::move(expr_stmt->expression_));
        default:
            return make_parser_err(syntax::ParserError::ILLEGAL_LABEL_EXPRESSION,
                                   expr_stmt->get_token());
        }
    }
    case NodeKind::BLOCK_STATEMENT: return Node::downcast<BlockStatement>(std::move(raw_stmt));
    default:
        return make_parser_err(syntax::ParserError::ILLEGAL_LABEL_STATEMENT, raw_stmt->get_token());
    }
}

} // namespace porpoise::ast

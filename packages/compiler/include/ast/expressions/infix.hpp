#pragma once

#include <utility>

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

template <typename Derived> class InfixExpression : public ExprBase<Derived> {
  public:
    explicit InfixExpression(const syntax::Token& start_token,
                             mem::Box<Expression> lhs,
                             syntax::TokenType    op,
                             mem::Box<Expression> rhs) noexcept
        : ExprBase<Derived>{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    MAKE_GETTER(lhs, const Expression&, *)
    MAKE_GETTER(op, syntax::TokenType)
    MAKE_GETTER(rhs, const Expression&, *)

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> lhs)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
        const auto op_token           = parser.current_token();
        const auto current_precedence = parser.poll_current_precedence();
        if (parser.peek_token_is(syntax::TokenType::END)) {
            return make_parser_unexpected(syntax::ParserError::INFIX_MISSING_RHS, op_token);
        }

        parser.advance();
        auto rhs = TRY(parser.parse_expression(current_precedence));
        return mem::make_box<Derived>(
            lhs->get_token(), std::move(lhs), op_token.type, std::move(rhs));
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return *lhs_ == *casted.lhs_ && op_ == casted.op_ && *rhs_ == *casted.rhs_;
    }

  protected:
    mem::Box<Expression> lhs_;
    syntax::TokenType    op_;
    mem::Box<Expression> rhs_;
};

#define DECLARE_INFIX_EXPRESSION(Type, Kind)    \
    class Type : public InfixExpression<Type> { \
      public:                                   \
        static constexpr auto KIND = Kind;      \
                                                \
      public:                                   \
        using InfixExpression::InfixExpression; \
        MAKE_AST_COPY_MOVE(Type)                \
                                                \
        using InfixExpression::parse;           \
    };

DECLARE_INFIX_EXPRESSION(AssignmentExpression, NodeKind::ASSIGNMENT_EXPRESSION)
DECLARE_INFIX_EXPRESSION(BinaryExpression, NodeKind::BINARY_EXPRESSION)
DECLARE_INFIX_EXPRESSION(DotExpression, NodeKind::DOT_EXPRESSION)
DECLARE_INFIX_EXPRESSION(RangeExpression, NodeKind::RANGE_EXPRESSION)

#undef DECLARE_INFIX_EXPRESSION

} // namespace porpoise::ast

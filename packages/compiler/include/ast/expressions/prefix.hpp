#pragma once

#include <utility>

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

template <typename Derived> class PrefixExpression : public ExprBase<Derived> {
  public:
    PrefixExpression(const syntax::Token& start_token, mem::Box<Expression> rhs) noexcept
        : ExprBase<Derived>{start_token}, rhs_{std::move(rhs)} {}

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
        const auto prefix_token = parser.current_token();
        if (parser.peek_token_is(syntax::TokenType::END)) {
            return make_parser_unexpected(syntax::ParserError::PREFIX_MISSING_OPERAND,
                                          prefix_token);
        }
        parser.advance();

        auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
        return mem::make_box<Derived>(prefix_token, std::move(operand));
    }

    [[nodiscard]] auto get_op() const noexcept -> syntax::TokenType {
        return this->start_token_.type;
    }
    MAKE_GETTER(rhs, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return *rhs_ == *casted.rhs_;
    }

  private:
    mem::Box<Expression> rhs_;
};

#define DECLARE_PREFIX_EXPRESSION(Type, Kind)     \
    class Type : public PrefixExpression<Type> {  \
      public:                                     \
        static constexpr auto KIND = Kind;        \
                                                  \
      public:                                     \
        using PrefixExpression::PrefixExpression; \
        MAKE_MOVE_CONSTRUCTABLE_ONLY(Type)        \
                                                  \
        using PrefixExpression::parse;            \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression, NodeKind::UNARY_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression, NodeKind::REFERENCE_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression, NodeKind::DEREFERENCE_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(ImplicitAccessExpression, NodeKind::IMPLICIT_ACCESS_EXPRESSION)

#undef DECLARE_PREFIX_EXPRESSION

} // namespace porpoise::ast

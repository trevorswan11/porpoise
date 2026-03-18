#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class ScopeResolutionExpression : public ExprBase<ScopeResolutionExpression> {
  public:
    static constexpr auto KIND = NodeKind::SCOPE_RESOLUTION_EXPRESSION;

  public:
    explicit ScopeResolutionExpression(const Token&              start_token,
                                       Box<Expression>           outer,
                                       Box<IdentifierExpression> inner) noexcept;
    ~ScopeResolutionExpression() override;

    MAKE_AST_COPY_MOVE(ScopeResolutionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> outer)
        -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(outer, const Expression&, *)
    MAKE_AST_GETTER(inner, const IdentifierExpression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           outer_;
    Box<IdentifierExpression> inner_;
};

} // namespace porpoise::ast

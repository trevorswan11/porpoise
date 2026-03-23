#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class ScopeResolutionExpression : public ExprBase<ScopeResolutionExpression> {
  public:
    static constexpr auto KIND = NodeKind::SCOPE_RESOLUTION_EXPRESSION;

  public:
    explicit ScopeResolutionExpression(const syntax::Token&           start_token,
                                       mem::Box<Expression>           outer,
                                       mem::Box<IdentifierExpression> inner) noexcept;
    ~ScopeResolutionExpression() override;

    MAKE_AST_COPY_MOVE(ScopeResolutionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> outer)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(outer, const Expression&, *)
    MAKE_GETTER(inner, const IdentifierExpression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<Expression>           outer_;
    mem::Box<IdentifierExpression> inner_;
};

} // namespace porpoise::ast

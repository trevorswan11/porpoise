#pragma once

#include <utility>

#include "ast/node.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

class ExpressionStatement : public StmtBase<ExpressionStatement> {
  public:
    static constexpr auto KIND = NodeKind::EXPRESSION_STATEMENT;

  public:
    ExpressionStatement(const syntax::Token& start_token, mem::Box<Expression> expression) noexcept
        : StmtBase{start_token}, expression_{std::move(expression)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ExpressionStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, bool require_semicolon)
        -> Result<mem::Box<Statement>, syntax::Diagnostic>;

    MAKE_GETTER(expression, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<ExpressionStatement>(other);
        return *expression_ == *casted.expression_;
    }

  private:
    mem::Box<Expression> expression_;

    // Label expression moves the expression out when parsing
    friend class LabelExpression;
};

} // namespace porpoise::ast

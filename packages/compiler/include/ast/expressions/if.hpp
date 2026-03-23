#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IfExpression : public ExprBase<IfExpression> {
  public:
    static constexpr auto KIND = NodeKind::IF_EXPRESSION;

  public:
    explicit IfExpression(const syntax::Token&          start_token,
                          mem::Box<Expression>          condition,
                          mem::Box<Statement>           consequence,
                          Optional<mem::Box<Statement>> alternate) noexcept
        : ExprBase{start_token}, condition_{std::move(condition)},
          consequence_{std::move(consequence)}, alternate_{std::move(alternate)} {}

    MAKE_AST_COPY_MOVE(IfExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(condition, const Expression&, *)
    MAKE_GETTER(consequence, const Statement&, *)
    MAKE_OPTIONAL_UNPACKER(alternate, Statement, alternate_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<IfExpression>(other);
        return *condition_ == *casted.condition_ && *consequence_ == *casted.consequence_ &&
               optional::unsafe_eq<Statement>(alternate_, casted.alternate_);
    }

  private:
    mem::Box<Expression>          condition_;
    mem::Box<Statement>           consequence_;
    Optional<mem::Box<Statement>> alternate_;
};

} // namespace porpoise::ast

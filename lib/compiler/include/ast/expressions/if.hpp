#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IfExpression : public ExprBase<IfExpression> {
  public:
    static constexpr auto KIND = NodeKind::IF_EXPRESSION;

  public:
    IfExpression(const syntax::Token&        start_token,
                 bool                        constexpr_condition,
                 mem::Box<Expression>        condition,
                 mem::Box<Statement>         consequence,
                 mem::NullableBox<Statement> alternate) noexcept
        : ExprBase{start_token}, constexpr_condition_{constexpr_condition},
          condition_{std::move(condition)}, consequence_{std::move(consequence)},
          alternate_{std::move(alternate)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(IfExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    [[nodiscard]] auto is_constexpr() const noexcept -> bool { return constexpr_condition_; }
    MAKE_GETTER(condition, const Expression&, *)
    MAKE_GETTER(consequence, const Statement&, *)
    MAKE_NULLABLE_BOX_UNPACKER(alternate, Statement, alternate_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted       = as<IfExpression>(other);
        const auto  constexpr_eq = constexpr_condition_ == casted.constexpr_condition_;
        return constexpr_eq && *condition_ == *casted.condition_ &&
               *consequence_ == *casted.consequence_ &&
               mem::nullable_boxes_eq(alternate_, casted.alternate_);
    }

  private:
    bool                        constexpr_condition_;
    mem::Box<Expression>        condition_;
    mem::Box<Statement>         consequence_;
    mem::NullableBox<Statement> alternate_;
};

} // namespace porpoise::ast

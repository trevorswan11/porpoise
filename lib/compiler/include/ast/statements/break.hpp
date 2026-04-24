#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class BreakStatement : public StmtBase<BreakStatement> {
  public:
    static constexpr auto KIND = NodeKind::BREAK_STATEMENT;

  public:
    BreakStatement(const syntax::Token&                   start_token,
                   mem::NullableBox<IdentifierExpression> label,
                   mem::NullableBox<Expression>           expression) noexcept;
    ~BreakStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(BreakStatement)

    auto                      accept(Visitor& v) const noexcept -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(expression, Expression, expression_, *)
    MAKE_NULLABLE_BOX_UNPACKER(label, IdentifierExpression, label_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::NullableBox<IdentifierExpression> label_;
    mem::NullableBox<Expression>           expression_;
};

} // namespace porpoise::ast

#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class BlockStatement;

class WhileLoopExpression : public ExprBase<WhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::WHILE_LOOP_EXPRESSION;

  public:
    WhileLoopExpression(const syntax::Token&         start_token,
                        mem::Box<Expression>         condition,
                        mem::NullableBox<Expression> continuation,
                        mem::Box<BlockStatement>     block,
                        mem::NullableBox<Statement>  non_break) noexcept;
    ~WhileLoopExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(WhileLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(condition, const Expression&, *)
    MAKE_NULLABLE_BOX_UNPACKER(continuation, Expression, continuation_, *)
    MAKE_GETTER(block, const BlockStatement&, *)
    MAKE_NULLABLE_BOX_UNPACKER(non_break, Statement, non_break_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<Expression>         condition_;
    mem::NullableBox<Expression> continuation_;
    mem::Box<BlockStatement>     block_;
    mem::NullableBox<Statement>  non_break_;
};

} // namespace porpoise::ast

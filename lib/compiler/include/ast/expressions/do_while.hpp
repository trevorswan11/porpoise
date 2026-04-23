#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class BlockStatement;

class DoWhileLoopExpression : public ExprBase<DoWhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::DO_WHILE_LOOP_EXPRESSION;

  public:
    DoWhileLoopExpression(const syntax::Token&     start_token,
                          mem::Box<BlockStatement> block,
                          mem::Box<Expression>     condition) noexcept;
    ~DoWhileLoopExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(DoWhileLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(block, const BlockStatement&, *)
    MAKE_GETTER(condition, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<BlockStatement> block_;
    mem::Box<Expression>     condition_;
};

} // namespace porpoise::ast

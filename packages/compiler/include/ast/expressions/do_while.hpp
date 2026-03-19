#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class BlockStatement;

class DoWhileLoopExpression : public ExprBase<DoWhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::DO_WHILE_LOOP_EXPRESSION;

  public:
    explicit DoWhileLoopExpression(const Token&        start_token,
                                   Box<BlockStatement> block,
                                   Box<Expression>     condition) noexcept;
    ~DoWhileLoopExpression() override;

    MAKE_AST_COPY_MOVE(DoWhileLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_GETTER(block, const BlockStatement&, *)
    MAKE_GETTER(condition, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<BlockStatement> block_;
    Box<Expression>     condition_;
};

} // namespace porpoise::ast

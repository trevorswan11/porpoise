#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class BlockStatement;

class WhileLoopExpression : public ExprBase<WhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::WHILE_LOOP_EXPRESSION;

  public:
    explicit WhileLoopExpression(const Token&              start_token,
                                 Box<Expression>           condition,
                                 Optional<Box<Expression>> continuation,
                                 Box<BlockStatement>       block,
                                 Optional<Box<Statement>>  non_break) noexcept;
    ~WhileLoopExpression() override;

    MAKE_AST_COPY_MOVE(WhileLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_GETTER(condition, const Expression&, *)
    MAKE_OPTIONAL_UNPACKER(continuation, Expression, continuation_, **)
    MAKE_GETTER(block, const BlockStatement&, *)
    MAKE_OPTIONAL_UNPACKER(non_break, Statement, non_break_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           condition_;
    Optional<Box<Expression>> continuation_;
    Box<BlockStatement>       block_;
    Optional<Box<Statement>>  non_break_;
};

} // namespace porpoise::ast

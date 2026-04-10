#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class BlockStatement;

class InfiniteLoopExpression : public ExprBase<InfiniteLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::INFINITE_LOOP_EXPRESSION;

  public:
    InfiniteLoopExpression(const syntax::Token&     start_token,
                           mem::Box<BlockStatement> block) noexcept;
    ~InfiniteLoopExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(InfiniteLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(block, const BlockStatement&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<BlockStatement> block_;
};

} // namespace porpoise::ast

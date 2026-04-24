#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class DoWhileLoopExpression;
class ForLoopExpression;
class IfExpression;
class InfiniteLoopExpression;
class MatchExpression;
class WhileLoopExpression;
class BlockStatement;

class LabelExpression : public ExprBase<LabelExpression> {
  public:
    using LabeledNode = std::variant<mem::Box<DoWhileLoopExpression>,
                                     mem::Box<ForLoopExpression>,
                                     mem::Box<IfExpression>,
                                     mem::Box<InfiniteLoopExpression>,
                                     mem::Box<MatchExpression>,
                                     mem::Box<WhileLoopExpression>,
                                     mem::Box<BlockStatement>>;

  public:
    static constexpr auto KIND = NodeKind::LABEL_EXPRESSION;

  public:
    LabelExpression(const syntax::Token&           start_token,
                    mem::Box<IdentifierExpression> name,
                    LabeledNode                    body) noexcept;
    ~LabelExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(LabelExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> name)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(name, const IdentifierExpression&, *)
    MAKE_VARIANT_UNPACKER(
        do_while_body, DoWhileLoopExpression, mem::Box<DoWhileLoopExpression>, body_, *std::get)
    MAKE_VARIANT_UNPACKER(
        for_loop_body, ForLoopExpression, mem::Box<ForLoopExpression>, body_, *std::get)
    MAKE_VARIANT_UNPACKER(if_body, IfExpression, mem::Box<IfExpression>, body_, *std::get)
    MAKE_VARIANT_UNPACKER(infinite_loop_body,
                          InfiniteLoopExpression,
                          mem::Box<InfiniteLoopExpression>,
                          body_,
                          *std::get)
    MAKE_VARIANT_UNPACKER(match_body, MatchExpression, mem::Box<MatchExpression>, body_, *std::get)
    MAKE_VARIANT_UNPACKER(
        while_body, WhileLoopExpression, mem::Box<WhileLoopExpression>, body_, *std::get)
    MAKE_VARIANT_UNPACKER(block_body, BlockStatement, mem::Box<BlockStatement>, body_, *std::get)
    MAKE_VARIANT_MATCHER(body_)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    [[nodiscard]] static auto deconstruct_body(mem::Box<ast::Statement>&& raw_stmt)
        -> Result<LabeledNode, syntax::ParserDiagnostic>;

  private:
    mem::Box<IdentifierExpression> name_;
    LabeledNode                    body_;
};

} // namespace porpoise::ast

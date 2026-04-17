#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class BlockStatement;
class IdentifierExpression;

class ForLoopCapture {
  public:
    class Valued {
      public:
        Valued(TypeModifier modifier, mem::Box<IdentifierExpression> ident) noexcept;
        ~Valued();

        MAKE_MOVE_CONSTRUCTABLE_ONLY(Valued)

        MAKE_GETTER(modifier, const TypeModifier&)
        MAKE_GETTER(ident, const IdentifierExpression&, *)

        MAKE_EQ_DELEGATION(Valued)

      private:
        TypeModifier                   modifier_;
        mem::Box<IdentifierExpression> ident_;
    };

  public:
    explicit ForLoopCapture(const syntax::Token& token) noexcept;
    explicit ForLoopCapture(Valued valued) noexcept;
    ~ForLoopCapture();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ForLoopCapture)

    auto accept(Visitor& v) const -> void;

    MAKE_VARIANT_UNPACKER(valued, Valued, Valued, underlying_, std::get)
    [[nodiscard]] auto is_discarded() const noexcept -> bool {
        return std::holds_alternative<syntax::Token>(underlying_);
    }

    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(ForLoopCapture)

  private:
    std::variant<Valued, syntax::Token> underlying_;
};

class ForLoopExpression : public ExprBase<ForLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::FOR_LOOP_EXPRESSION;

  public:
    ForLoopExpression(const syntax::Token&              start_token,
                      std::vector<mem::Box<Expression>> iterables,
                      std::vector<ForLoopCapture>       captures,
                      mem::Box<BlockStatement>          block,
                      mem::NullableBox<Statement>       non_break) noexcept;
    ~ForLoopExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ForLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(iterables, std::span<const mem::Box<Expression>>)
    MAKE_GETTER(captures, std::span<const ForLoopCapture>)
    MAKE_GETTER(block, const BlockStatement&, *)
    MAKE_NULLABLE_BOX_UNPACKER(non_break, Statement, non_break_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<mem::Box<Expression>> iterables_;
    std::vector<ForLoopCapture>       captures_;
    mem::Box<BlockStatement>          block_;
    mem::NullableBox<Statement>       non_break_;
};

} // namespace porpoise::ast

#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace conch::ast {

class BlockStatement;
class IdentifierExpression;

class ForLoopCapture {
  public:
    class Valued {
      public:
        explicit Valued(TypeModifier modifier, Box<IdentifierExpression> ident) noexcept;
        ~Valued();

        MAKE_AST_COPY_MOVE(Valued)

        MAKE_AST_GETTER(modifier, const TypeModifier&, )
        MAKE_AST_GETTER(ident, const IdentifierExpression&, *)

        MAKE_AST_DEPENDENT_EQ(Valued)

      private:
        TypeModifier              modifier_;
        Box<IdentifierExpression> ident_;
    };

  public:
    explicit ForLoopCapture() noexcept;
    explicit ForLoopCapture(Valued valued) noexcept;
    ~ForLoopCapture();

    MAKE_AST_COPY_MOVE(ForLoopCapture)

    MAKE_VARIANT_UNPACKER(valued, Valued, Valued, underlying_, std::get)
    [[nodiscard]] auto is_discarded() const noexcept -> bool {
        return std::holds_alternative<std::monostate>(underlying_);
    }

    MAKE_AST_DEPENDENT_EQ(ForLoopCapture)

  private:
    std::variant<Valued, std::monostate> underlying_;
};

class ForLoopExpression : public ExprBase<ForLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::FOR_LOOP_EXPRESSION;

  public:
    explicit ForLoopExpression(const Token&                 start_token,
                               std::vector<Box<Expression>> iterables,
                               std::vector<ForLoopCapture>  captures,
                               Box<BlockStatement>          block,
                               Optional<Box<Statement>>     non_break) noexcept;
    ~ForLoopExpression() override;

    MAKE_AST_COPY_MOVE(ForLoopExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(iterables, std::span<const Box<Expression>>, )
    MAKE_AST_GETTER(captures, std::span<const ForLoopCapture>, )
    MAKE_AST_GETTER(block, const BlockStatement&, *)
    MAKE_OPTIONAL_UNPACKER(non_break, Statement, non_break_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<Box<Expression>> iterables_;
    std::vector<ForLoopCapture>  captures_;
    Box<BlockStatement>          block_;
    Optional<Box<Statement>>     non_break_;
};

} // namespace conch::ast

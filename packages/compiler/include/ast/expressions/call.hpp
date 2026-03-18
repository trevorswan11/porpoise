#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class CallArgument {
  public:
    explicit CallArgument(Box<Expression> argument) noexcept;
    explicit CallArgument(ExplicitType&& argument) noexcept;
    ~CallArgument();

    MAKE_AST_COPY_MOVE(CallArgument)

    MAKE_VARIANT_UNPACKER(expression, Expression, Box<Expression>, argument_, *std::get)
    MAKE_VARIANT_UNPACKER(type, ExplicitType, ExplicitType, argument_, std::get)

    MAKE_AST_DEPENDENT_EQ(CallArgument)

  private:
    std::variant<Box<Expression>, ExplicitType> argument_;
};

class CallExpression : public ExprBase<CallExpression> {
  public:
    static constexpr auto KIND = NodeKind::CALL_EXPRESSION;

  public:
    explicit CallExpression(const Token&              start_token,
                            Box<Expression>           function,
                            std::vector<CallArgument> arguments) noexcept;
    ~CallExpression() override;

    MAKE_AST_COPY_MOVE(CallExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> function)
        -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(function, const Expression&, *)
    MAKE_AST_GETTER(arguments, std::span<const CallArgument>, )

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           function_;
    std::vector<CallArgument> arguments_;
};

} // namespace porpoise::ast

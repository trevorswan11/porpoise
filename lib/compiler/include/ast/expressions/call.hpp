#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class CallArgument {
  public:
    explicit CallArgument(mem::Box<Expression> argument) noexcept;
    explicit CallArgument(ExplicitType&& argument) noexcept;
    ~CallArgument();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(CallArgument)

    auto accept(Visitor& v) const -> void;

    MAKE_VARIANT_UNPACKER(expression, Expression, mem::Box<Expression>, argument_, *std::get)
    MAKE_VARIANT_UNPACKER(type, ExplicitType, ExplicitType, argument_, std::get)

    MAKE_EQ_DELEGATION(CallArgument)

  private:
    std::variant<mem::Box<Expression>, ExplicitType> argument_;
};

class CallExpression : public ExprBase<CallExpression> {
  public:
    static constexpr auto KIND = NodeKind::CALL_EXPRESSION;

  public:
    CallExpression(const syntax::Token&      start_token,
                   mem::Box<Expression>      function,
                   std::vector<CallArgument> arguments) noexcept;
    ~CallExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(CallExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> function)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(function, const Expression&, *)
    MAKE_GETTER(arguments, std::span<const CallArgument>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<Expression>      function_;
    std::vector<CallArgument> arguments_;
};

} // namespace porpoise::ast

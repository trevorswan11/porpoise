#pragma once

#include "ast/expressions/prefix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class UnaryExpression : public PrefixExpression<UnaryExpression> {
  public:
    static constexpr auto KIND = NodeKind::UNARY_EXPRESSION;

  public:
    using PrefixExpression::parse;
    using PrefixExpression::PrefixExpression;
};

} // namespace conch::ast

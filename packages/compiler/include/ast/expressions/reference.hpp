#pragma once

#include "ast/expressions/prefix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class ReferenceExpression : public PrefixExpression<ReferenceExpression> {
  public:
    static constexpr auto KIND = NodeKind::REFERENCE_EXPRESSION;

  public:
    using PrefixExpression::parse;
    using PrefixExpression::PrefixExpression;
};

class DereferenceExpression : public PrefixExpression<DereferenceExpression> {
  public:
    static constexpr auto KIND = NodeKind::DEREFERENCE_EXPRESSION;

  public:
    using PrefixExpression::parse;
    using PrefixExpression::PrefixExpression;
};

} // namespace conch::ast

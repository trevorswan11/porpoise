#pragma once

#include "ast/oop_node.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

class GroupedExpression {
  public:
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic> {
        parser.advance();
        auto inner = TRY(parser.parse_expression());
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
        return inner;
    }
};

} // namespace porpoise::ast

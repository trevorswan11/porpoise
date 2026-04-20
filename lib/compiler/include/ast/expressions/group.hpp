#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class GroupedExpression {
  public:
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
        parser.advance();
        auto inner = TRY(parser.parse_expression());
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
        return inner;
    }
};

} // namespace porpoise::ast

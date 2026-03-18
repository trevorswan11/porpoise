#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class GroupedExpression {
  public:
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
        parser.advance();
        auto inner = TRY(parser.parse_expression());
        TRY(parser.expect_peek(TokenType::RPAREN));
        return inner;
    }
};

} // namespace porpoise::ast

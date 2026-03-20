#include "ast/expressions/index.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IndexExpression::parse(syntax::Parser& parser, Box<Expression> array)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = array->get_token();
    if (parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        return make_parser_unexpected(syntax::ParserError::INDEX_MISSING_EXPRESSION, start_token);
    }
    parser.advance();

    auto idx_expr = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    return make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));
}

} // namespace porpoise::ast

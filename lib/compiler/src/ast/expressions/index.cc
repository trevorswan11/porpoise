#include "ast/expressions/index.hh"

#include "ast/visitor.hh"

namespace porpoise::ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IndexExpression::parse(syntax::Parser& parser, mem::Box<Expression> array)
    -> Result<mem::Box<Expression>, syntax::Diagnostic> {
    const auto start_token = array->get_token();
    if (parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        return make_syntax_err(syntax::Error::INDEX_MISSING_EXPRESSION, start_token);
    }
    parser.advance();

    auto idx_expr = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    return mem::make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));
}

} // namespace porpoise::ast

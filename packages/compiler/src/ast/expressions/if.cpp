#include "ast/expressions/if.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IfExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::IF_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // The consequence and alternate are trivially handled by restricted statement parsers
    parser.advance();
    auto consequence =
        TRY(parser.parse_restricted_statement(syntax::ParserError::ILLEGAL_IF_BRANCH));
    auto alternate =
        TRY(parser.try_parse_restricted_alternate(syntax::ParserError::ILLEGAL_IF_BRANCH));

    return mem::make_box<IfExpression>(
        start_token, std::move(condition), std::move(consequence), std::move(alternate));
}

} // namespace porpoise::ast

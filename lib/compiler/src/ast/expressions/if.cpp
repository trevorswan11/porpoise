#include "ast/expressions/if.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IfExpression::parse(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    bool constexpr_condition = false;
    if (parser.peek_token_is(syntax::TokenType::CONSTEXPR)) {
        constexpr_condition = true;
        parser.advance();
    }

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_err(syntax::ParserError::IF_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // The consequence and alternate are trivially handled by restricted statement parsers
    parser.advance();
    auto consequence =
        TRY(parser.parse_restricted_statement(syntax::ParserError::ILLEGAL_IF_BRANCH, false));
    auto alternate =
        TRY(parser.try_parse_restricted_alternate(syntax::ParserError::ILLEGAL_IF_BRANCH, false));

    return mem::make_box<IfExpression>(start_token,
                                       constexpr_condition,
                                       std::move(condition),
                                       std::move(consequence),
                                       std::move(alternate));
}

} // namespace porpoise::ast

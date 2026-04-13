#include "ast/statements/discard.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto DiscardStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DiscardStatement::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_parser_unexpected(syntax::ParserError::DISCARD_MISSING_DISCARDEE,
                                      parser.get_current_token());
    }

    parser.advance();
    auto expr = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return mem::make_box<DiscardStatement>(start_token, std::move(expr));
}

} // namespace porpoise::ast

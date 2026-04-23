#include "ast/statements/expression.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ExpressionStatement::parse(syntax::Parser& parser, [[maybe_unused]] bool require_semicolon)
    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    auto       expr        = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
            parser.advance();
        } else if (require_semicolon) {
            TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
        }
    }
    return mem::make_box<ExpressionStatement>(start_token, std::move(expr));
}

} // namespace porpoise::ast

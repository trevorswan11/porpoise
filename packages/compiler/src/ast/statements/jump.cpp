#include "ast/statements/jump.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto JumpStatement::parse(syntax::Parser& parser)
    -> Expected<Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    Optional<Box<Expression>> value;
    if (start_token.type == syntax::TokenType::RETURN &&
        !parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value = TRY(parser.parse_expression());
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return make_box<JumpStatement>(start_token, std::move(value));
}

} // namespace porpoise::ast

#include "ast/statements/return.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto ReturnStatement::accept(Visitor& v) const noexcept -> void { v.visit(*this); }

auto ReturnStatement::parse(syntax::Parser& parser)
    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    mem::NullableBox<Expression> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value = mem::nullable_box_from(TRY(parser.parse_expression()));
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return mem::make_box<ReturnStatement>(start_token, std::move(value));
}

} // namespace porpoise::ast

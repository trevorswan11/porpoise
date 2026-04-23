#include "ast/statements/jump.hpp"

#include "ast/expressions/identifier.hpp"

namespace porpoise::ast {

JumpStatement::JumpStatement(const syntax::Token&                   start_token,
                             mem::NullableBox<IdentifierExpression> label,
                             mem::NullableBox<Expression>           expression) noexcept
    : JumpStmtBase{start_token, std::move(expression)}, label_{std::move(label)} {}
JumpStatement::~JumpStatement() = default;

auto JumpStatement::parse(syntax::Parser& parser)
    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    // Labels are optional
    mem::NullableBox<IdentifierExpression> label;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        label = mem::nullable_box_from(
            Node::downcast<ast::IdentifierExpression>(TRY(IdentifierExpression::parse(parser))));
    }

    // Values can be present but must be associated with a label
    mem::NullableBox<Expression> value;
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        parser.advance();
        value = mem::nullable_box_from(TRY(parser.parse_expression()));
    }

    if (value && !label) {
        return make_parser_err(syntax::ParserError::VALUED_JUMP_MISSING_LABEL, start_token);
    }
    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return mem::make_box<JumpStatement>(start_token, std::move(label), std::move(value));
}

auto JumpStatement::is_continue() const noexcept -> bool {
    return start_token_.type == syntax::TokenType::CONTINUE;
}

auto JumpStatement::is_break() const noexcept -> bool {
    return start_token_.type == syntax::TokenType::BREAK;
}

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

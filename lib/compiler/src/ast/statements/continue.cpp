#include "ast/statements/continue.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

ContinueStatement::ContinueStatement(const syntax::Token&                   start_token,
                                     mem::NullableBox<IdentifierExpression> label) noexcept
    : StmtBase{start_token}, label_{std::move(label)} {}
ContinueStatement::~ContinueStatement() = default;

auto ContinueStatement::accept(Visitor& v) const noexcept -> void { v.visit(*this); }

auto ContinueStatement::parse(syntax::Parser& parser)
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

    // Values can never be present in a continue
    if (!parser.peek_token_is(syntax::TokenType::END) &&
        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_parser_err(syntax::ParserError::VALUED_CONTINUE, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return mem::make_box<ContinueStatement>(start_token, std::move(label));
}

auto ContinueStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = Node::as<ContinueStatement>(other);
    return mem::nullable_boxes_eq(label_, casted.label_);
}

} // namespace porpoise::ast

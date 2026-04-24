#include "ast/statements/break.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

BreakStatement::BreakStatement(const syntax::Token&                   start_token,
                               mem::NullableBox<IdentifierExpression> label,
                               mem::NullableBox<Expression>           expression) noexcept
    : StmtBase{start_token}, label_{std::move(label)}, expression_{std::move(expression)} {}
BreakStatement::~BreakStatement() = default;

auto BreakStatement::accept(Visitor& v) const noexcept -> void { v.visit(*this); }

auto BreakStatement::parse(syntax::Parser& parser)
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
        return make_parser_err(syntax::ParserError::VALUED_BREAK_MISSING_LABEL, start_token);
    }
    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return mem::make_box<BreakStatement>(start_token, std::move(label), std::move(value));
}

auto BreakStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = Node::as<BreakStatement>(other);
    return mem::nullable_boxes_eq(label_, casted.label_) &&
           mem::nullable_boxes_eq(expression_, casted.expression_);
}

} // namespace porpoise::ast

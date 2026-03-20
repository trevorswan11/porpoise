#include "ast/statements/expression.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ExpressionStatement::parse(syntax::Parser& parser)
    -> Expected<Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    auto       expr        = TRY(parser.parse_expression());

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return make_box<ExpressionStatement>(start_token, std::move(expr));
}

} // namespace porpoise::ast

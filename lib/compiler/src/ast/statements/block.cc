#include "ast/statements/block.hh"

#include "ast/visitor.hh"

namespace porpoise::ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(syntax::Parser& parser)
    -> Result<mem::Box<Statement>, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();

    Statements statements;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        statements.emplace_back(TRY(parser.parse_statement(true)));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return mem::make_box<BlockStatement>(start_token, std::move(statements));
}

} // namespace porpoise::ast

#include "ast/statements/defer.hh"

#include "ast/statements/block.hh"
#include "ast/statements/discard.hh"
#include "ast/statements/expression.hh"
#include "ast/visitor.hh"

namespace porpoise::ast {

auto DeferStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeferStatement::parse(syntax::Parser& parser)
    -> Result<mem::Box<Statement>, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.peek_token_is(syntax::TokenType::END) ||
        parser.peek_token_is(syntax::TokenType::SEMICOLON)) {
        return make_syntax_err(syntax::Error::DEFER_MISSING_DEFERREE, start_token);
    }
    parser.advance();
    auto stmt = TRY(parser.parse_statement(true));

    // The statement has different restrictions from expression alternates
    if (!stmt->any<ExpressionStatement, DiscardStatement, BlockStatement>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_DEFERRED_STATEMENT, stmt->get_token());
    }
    return mem::make_box<DeferStatement>(start_token, std::move(stmt));
}

} // namespace porpoise::ast

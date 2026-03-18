#include "ast/statements/defer.hpp"

#include "ast/statements/block.hpp"
#include "ast/statements/discard.hpp"
#include "ast/statements/expression.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

auto DeferStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeferStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (parser.peek_token_is(TokenType::END) || parser.peek_token_is(TokenType::SEMICOLON)) {
        return make_parser_unexpected(ParserError::DEFER_MISSING_DEFERREE, start_token);
    }
    parser.advance();
    auto stmt = TRY(parser.parse_statement());

    // The statement has different restrictions from expression alternates
    if (!stmt->any<ExpressionStatement, DiscardStatement, BlockStatement>()) {
        return make_parser_unexpected(ParserError::ILLEGAL_DEFERRED_STATEMENT, stmt->get_token());
    }
    return make_box<DeferStatement>(start_token, std::move(stmt));
}

} // namespace porpoise::ast

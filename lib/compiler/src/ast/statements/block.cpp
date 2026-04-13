#include "ast/statements/block.hpp"

#include "ast/statements/import.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(syntax::Parser& parser, bool allow_imports)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    Statements statements;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        auto inner_stmt = TRY(parser.parse_statement());

        // Import statements are usually only allowed at the top level
        if (!allow_imports && inner_stmt->is<ImportStatement>()) {
            return make_parser_unexpected(syntax::ParserError::ILLEGAL_BLOCK_STATEMENT,
                                          inner_stmt->get_token());
        }
        statements.emplace_back(std::move(inner_stmt));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return mem::make_box<BlockStatement>(start_token, std::move(statements));
}

} // namespace porpoise::ast

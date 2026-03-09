#include "ast/statements/block.hpp"

#include "ast/statements/import.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

auto BlockStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto BlockStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::vector<Box<Statement>> statements;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        auto inner_stmt = TRY(parser.parse_statement());

        // Import statements are only allowed at the top level
        if (inner_stmt->is<ImportStatement>()) {
            return make_parser_unexpected(ParserError::ILLEGAL_BLOCK_STATEMENT,
                                          inner_stmt->get_token());
        }
        statements.emplace_back(std::move(inner_stmt));
    }
    TRY(parser.expect_peek(TokenType::RBRACE));

    return make_box<BlockStatement>(start_token, std::move(statements));
}

} // namespace conch::ast

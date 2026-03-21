#include "ast/expressions/while.hpp"

#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

WhileLoopExpression::WhileLoopExpression(const syntax::Token&      start_token,
                                         Box<Expression>           condition,
                                         Optional<Box<Expression>> continuation,
                                         Box<BlockStatement>       block,
                                         Optional<Box<Statement>>  non_break) noexcept
    : ExprBase{start_token}, condition_{std::move(condition)},
      continuation_{std::move(continuation)}, block_{std::move(block)},
      non_break_{std::move(non_break)} {}
WhileLoopExpression::~WhileLoopExpression() = default;

auto WhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto WhileLoopExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::WHILE_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // Continuation expression is optional and is handled as in zig
    Optional<Box<Expression>> continuation;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        const auto continuation_start = parser.current_token();
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::LPAREN));

        // Consume again to look at the actual expr start
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::RPAREN)) {
            return make_parser_unexpected(syntax::ParserError::EMPTY_WHILE_CONTINUATION,
                                          continuation_start);
        }

        continuation.emplace(TRY(parser.parse_expression()));
        TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    }

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    auto non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::ParserError::ILLEGAL_LOOP_NON_BREAK));

    // There needs to be at least a continuation or block
    if (!continuation && block->empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_WHILE_LOOP, block->get_token());
    }

    return make_box<WhileLoopExpression>(start_token,
                                         std::move(condition),
                                         std::move(continuation),
                                         std::move(block),
                                         std::move(non_break));
}

auto WhileLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<WhileLoopExpression>(other);
    return *condition_ == *casted.condition_ &&
           optional::unsafe_eq<Expression>(continuation_, casted.continuation_) &&
           *block_ == *casted.block_ &&
           optional::unsafe_eq<Statement>(non_break_, casted.non_break_);
}

} // namespace porpoise::ast

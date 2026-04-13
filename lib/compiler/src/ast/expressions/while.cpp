#include "ast/expressions/while.hpp"

#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

WhileLoopExpression::WhileLoopExpression(const syntax::Token&         start_token,
                                         mem::Box<Expression>         condition,
                                         mem::NullableBox<Expression> continuation,
                                         mem::Box<BlockStatement>     block,
                                         mem::NullableBox<Statement>  non_break) noexcept
    : ExprBase{start_token}, condition_{std::move(condition)},
      continuation_{std::move(continuation)}, block_{std::move(block)},
      non_break_{std::move(non_break)} {}
WhileLoopExpression::~WhileLoopExpression() = default;

auto WhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto WhileLoopExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::WHILE_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // Continuation expression is optional and is handled as in zig
    mem::NullableBox<Expression> continuation;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        const auto continuation_start = parser.get_current_token();
        parser.advance();
        TRY(parser.expect_peek(syntax::TokenType::LPAREN));

        // Consume again to look at the actual expr start
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::RPAREN)) {
            return make_parser_unexpected(syntax::ParserError::EMPTY_WHILE_CONTINUATION,
                                          continuation_start);
        }

        continuation = mem::nullable_box_from(TRY(parser.parse_expression()));
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

    return mem::make_box<WhileLoopExpression>(start_token,
                                              std::move(condition),
                                              std::move(continuation),
                                              std::move(block),
                                              std::move(non_break));
}

auto WhileLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<WhileLoopExpression>(other);
    return *condition_ == *casted.condition_ &&
           mem::nullable_boxes_eq(continuation_, casted.continuation_) &&
           *block_ == *casted.block_ && mem::nullable_boxes_eq(non_break_, casted.non_break_);
}

} // namespace porpoise::ast

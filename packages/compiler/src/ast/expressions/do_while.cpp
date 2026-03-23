#include "ast/expressions/do_while.hpp"

#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

DoWhileLoopExpression::DoWhileLoopExpression(const syntax::Token&     start_token,
                                             mem::Box<BlockStatement> block,
                                             mem::Box<Expression>     condition) noexcept
    : ExprBase{start_token}, block_{std::move(block)}, condition_{std::move(condition)} {}
DoWhileLoopExpression::~DoWhileLoopExpression() = default;

auto DoWhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DoWhileLoopExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    TRY(parser.expect_peek(syntax::TokenType::WHILE));
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));

    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::WHILE_MISSING_CONDITION,
                                      parser.current_token());
    }
    parser.advance();

    // There's no continuation or non break clause so this is easy :)
    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));
    if (block->empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_LOOP, block->get_token());
    }
    return mem::make_box<DoWhileLoopExpression>(
        start_token, std::move(block), std::move(condition));
}

auto DoWhileLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<DoWhileLoopExpression>(other);
    return *block_ == *casted.block_ && *condition_ == *casted.condition_;
}

} // namespace porpoise::ast

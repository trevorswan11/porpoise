#include "ast/expressions/infinite_loop.hpp"

#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

InfiniteLoopExpression::InfiniteLoopExpression(const syntax::Token& start_token,
                                               Box<BlockStatement>  block) noexcept
    : ExprBase{start_token}, block_{std::move(block)} {}
InfiniteLoopExpression::~InfiniteLoopExpression() = default;

auto InfiniteLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto InfiniteLoopExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    if (block->empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_LOOP, block->get_token());
    }
    return make_box<InfiniteLoopExpression>(start_token, std::move(block));
}

auto InfiniteLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<InfiniteLoopExpression>(other);
    return *block_ == *casted.block_;
}

} // namespace porpoise::ast

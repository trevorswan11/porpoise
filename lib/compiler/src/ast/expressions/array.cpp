#include <algorithm>

#include "ast/expressions/array.hpp"

// IWYU pragma: begin_keeps
#include "ast/expressions/call.hpp"
#include "ast/expressions/enum.hpp"
#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/scope_resolve.hpp"
#include "ast/expressions/struct.hpp"
#include "ast/expressions/union.hpp"
// IWYU pragma: end_keeps

#include "ast/visitor.hpp"

namespace porpoise::ast {

ArrayExpression::ArrayExpression(const syntax::Token&              start_token,
                                 mem::NullableBox<Expression>      size,
                                 bool                              null_terminated,
                                 ExplicitType&&                    item_type,
                                 std::vector<mem::Box<Expression>> items) noexcept
    : ExprBase{start_token}, size_{std::move(size)}, null_terminated_{null_terminated},
      item_type_{std::move(item_type)}, items_{std::move(items)} {}
ArrayExpression::~ArrayExpression() = default;

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ArrayExpression::parse(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    auto                         null_terminated = false;
    mem::NullableBox<Expression> size;
    if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
        parser.advance();
        null_terminated = true;
    } else if (!parser.peek_token_is(syntax::TokenType::RBRACKET)) {
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::RBRACKET)) {
            return make_parser_err(syntax::ParserError::MISSING_ARRAY_SIZE_TOKEN, start_token);
        } else if (!parser.current_token_is(syntax::TokenType::UNDERSCORE)) {
            size = mem::nullable_box_from(TRY(parser.parse_expression()));
        }

        // The null terminated marker comes after the size for explicitly sized types
        if (parser.peek_token_is(syntax::TokenType::NULL_TERMINATED)) {
            parser.advance();
            null_terminated = true;
        }
    } else {
        // There needs to be a token for the size for array literals
        return make_parser_err(syntax::ParserError::MISSING_ARRAY_SIZE_TOKEN, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));
    auto item_type = TRY(ExplicitType::parse(parser));
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    // Current token is either the LBRACE at the start or a comma before parsing
    std::vector<mem::Box<Expression>> items;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        items.emplace_back(TRY(parser.parse_expression()));
        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return mem::make_box<ArrayExpression>(
        start_token, std::move(size), null_terminated, std::move(item_type), std::move(items));
}

auto ArrayExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ArrayExpression>(other);
    return mem::nullable_boxes_eq(size_, casted.size_) && item_type_ == casted.item_type_ &&
           null_terminated_ == casted.null_terminated_ &&
           std::ranges::equal(
               items_, casted.items_, [](const auto& a, const auto& b) { return *a == *b; });
}

} // namespace porpoise::ast

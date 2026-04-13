#include <algorithm>

#include "ast/expressions/for.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

ForLoopCapture::Valued::Valued(TypeModifier modifier, mem::Box<IdentifierExpression> ident) noexcept
    : modifier_{std::move(modifier)}, ident_{std::move(ident)} {}
ForLoopCapture::Valued::~Valued() = default;

auto ForLoopCapture::Valued::is_equal(const Valued& other) const noexcept -> bool {
    return modifier_ == other.modifier_ && *ident_ == *other.ident_;
}

ForLoopCapture::ForLoopCapture(const syntax::Token& token) noexcept : underlying_{token} {}
ForLoopCapture::ForLoopCapture(Valued valued) noexcept : underlying_{Valued{std::move(valued)}} {}
ForLoopCapture::~ForLoopCapture() = default;

auto ForLoopCapture::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopCapture::get_token() const noexcept -> const syntax::Token& {
    return std::visit(
        Overloaded{[](const syntax::Token& tok) -> const syntax::Token& { return tok; },
                   [](const Valued& valued) -> const syntax::Token& {
                       return valued.get_ident().get_token();
                   }},
        underlying_);
}

auto ForLoopCapture::is_equal(const ForLoopCapture& other) const noexcept -> bool {
    if (underlying_.index() != other.underlying_.index()) { return false; }
    return std::visit(
        Overloaded{[&other](const syntax::Token& tok) {
                       const auto& other_token = std::get<syntax::Token>(other.underlying_);
                       if (tok.type != other_token.type) { return false; }
                       if (tok.slice != other_token.slice) { return false; }
                       return true;
                   },
                   [&other](const Valued& valued) { return valued == other.get_valued(); }},
        underlying_);
}

ForLoopExpression::ForLoopExpression(const syntax::Token&              start_token,
                                     std::vector<mem::Box<Expression>> iterables,
                                     std::vector<ForLoopCapture>       captures,
                                     mem::Box<BlockStatement>          block,
                                     mem::NullableBox<Statement>       non_break) noexcept
    : ExprBase{start_token}, iterables_{std::move(iterables)}, captures_{std::move(captures)},
      block_{std::move(block)}, non_break_{std::move(non_break)} {}

ForLoopExpression::~ForLoopExpression() = default;

auto ForLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    // Iterables have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    if (parser.peek_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::FOR_MISSING_ITERABLES, start_token);
    }

    std::vector<mem::Box<Expression>> iterables;
    while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();

        auto iterable = TRY(parser.parse_expression());
        iterables.emplace_back(std::move(iterable));

        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    // Captures take on something similar to zig's capture syntax
    std::vector<ForLoopCapture> captures;
    TRY(parser.expect_peek(syntax::TokenType::BW_OR));
    while (!parser.peek_token_is(syntax::TokenType::BW_OR) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();
        if (parser.current_token_is(syntax::TokenType::UNDERSCORE)) {
            captures.emplace_back(parser.get_current_token());
        } else {
            // Always check for a modifier and advance past it if present
            const auto modifier = TypeModifier::from_token(parser.get_current_token());
            if (!modifier.is_value()) { parser.advance(); }

            auto capture = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
            captures.emplace_back(ForLoopCapture::Valued{std::move(modifier), std::move(capture)});
        }

        if (!parser.peek_token_is(syntax::TokenType::BW_OR)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::BW_OR));

    // Loops must have a well formed block and may have an alternate in non-break cases
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto block = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    auto non_break =
        TRY(parser.try_parse_restricted_alternate(syntax::ParserError::ILLEGAL_LOOP_NON_BREAK));

    // The number of captures must align with the number of iterables
    if (captures.size() != iterables.size()) {
        return make_parser_unexpected(syntax::ParserError::FOR_ITERABLE_CAPTURE_MISMATCH,
                                      start_token);
    }

    if (block->empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_FOR_LOOP, block->get_token());
    }

    return mem::make_box<ForLoopExpression>(start_token,
                                            std::move(iterables),
                                            std::move(captures),
                                            std::move(block),
                                            std::move(non_break));
}

auto ForLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted       = as<ForLoopExpression>(other);
    const auto  iterables_eq = std::ranges::equal(
        iterables_, casted.iterables_, [](const auto& a, const auto& b) { return *a == *b; });
    const auto captures_eq = std::ranges::equal(captures_, casted.captures_);
    return iterables_eq && captures_eq && *block_ == *casted.block_ &&
           mem::nullable_boxes_eq(non_break_, casted.non_break_);
}

} // namespace porpoise::ast

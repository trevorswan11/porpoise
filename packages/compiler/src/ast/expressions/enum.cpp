#include <algorithm>

#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

Enumeration::Enumeration(mem::Box<IdentifierExpression> ident,
                         Optional<mem::Box<Expression>> value) noexcept
    : ident_{std::move(ident)}, value_{std::move(value)} {}
Enumeration::~Enumeration() = default;

auto Enumeration::accept(Visitor& v) const -> void { v.visit(*this); }

[[nodiscard]] auto Enumeration::get_token() const noexcept -> const syntax::Token& {
    return ident_->get_token();
}

auto Enumeration::is_equal(const Enumeration& other) const noexcept -> bool {
    return *ident_ == *other.ident_ && optional::unsafe_eq<Expression>(value_, other.value_);
}

EnumExpression::EnumExpression(const syntax::Token&                     start_token,
                               Optional<mem::Box<IdentifierExpression>> underlying,
                               Enumerations                             enumerations) noexcept
    : ExprBase{start_token}, underlying_{std::move(underlying)},
      enumerations_{std::move(enumerations)} {}
EnumExpression::~EnumExpression() = default;

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto EnumExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    Optional<mem::Box<IdentifierExpression>> underlying;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance(2);
        underlying.emplace(
            downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser))));
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    if (parser.peek_token_is(syntax::TokenType::RBRACE)) {
        const auto opening = parser.current_token();
        parser.advance();
        return make_parser_unexpected(syntax::ParserError::ENUM_MISSING_VARIANTS, opening);
    }

    Enumerations enumeration;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        auto ident = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

        Optional<mem::Box<Expression>> value;
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance(2);
            value.emplace(TRY(parser.parse_expression()));
        }
        enumeration.emplace_back(std::move(ident), std::move(value));

        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return mem::make_box<EnumExpression>(
        start_token, std::move(underlying), std::move(enumeration));
}

auto EnumExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<EnumExpression>(other);
    return optional::unsafe_eq<IdentifierExpression>(underlying_, casted.underlying_) &&
           std::ranges::equal(enumerations_, casted.enumerations_);
}

} // namespace porpoise::ast

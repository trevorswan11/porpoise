#include <algorithm>

#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/declaration.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

Enumeration::Enumeration(mem::Box<IdentifierExpression> ident,
                         mem::NullableBox<Expression>   value) noexcept
    : ident_{std::move(ident)}, value_{std::move(value)} {}
Enumeration::~Enumeration() = default;

auto Enumeration::accept(Visitor& v) const -> void { v.visit(*this); }

[[nodiscard]] auto Enumeration::get_token() const noexcept -> const syntax::Token& {
    return ident_->get_token();
}

auto Enumeration::is_equal(const Enumeration& other) const noexcept -> bool {
    return *ident_ == *other.ident_ && mem::nullable_boxes_eq(value_, other.value_);
}

EnumExpression::EnumExpression(const syntax::Token&                   start_token,
                               mem::NullableBox<IdentifierExpression> underlying,
                               Enumerations                           enumerations,
                               Members                                members) noexcept
    : ExprBase{start_token}, underlying_{std::move(underlying)},
      enumerations_{std::move(enumerations)}, members_{std::move(members)} {}
EnumExpression::~EnumExpression() = default;

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto EnumExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();

    mem::NullableBox<IdentifierExpression> underlying;
    if (parser.peek_token_is(syntax::TokenType::COLON)) {
        parser.advance(2);
        underlying = mem::nullable_box_from(
            downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser))));
    }
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    Enumerations enumerations;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.get_peek_token().is_decl_token()) { break; }

        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        auto ident = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

        mem::NullableBox<Expression> value;
        if (parser.peek_token_is(syntax::TokenType::ASSIGN)) {
            parser.advance(2);
            value = mem::nullable_box_from(TRY(parser.parse_expression()));
        }
        enumerations.emplace_back(std::move(ident), std::move(value));

        // No comma means that its the end or that there is a decl list starting
        if (!parser.peek_token_is(syntax::TokenType::COMMA)) { break; }
        parser.advance();
    }

    auto members = TRY(parser.parse_member_decls(validate_non_struct_member));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    // Validate here so that there aren't 3 errors spawning from an empty enum with decls
    if (enumerations.empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_ENUM, start_token);
    }
    return mem::make_box<EnumExpression>(
        start_token, std::move(underlying), std::move(enumerations), std::move(members));
}

auto EnumExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted          = as<EnumExpression>(other);
    const auto  enumerations_eq = std::ranges::equal(enumerations_, casted.enumerations_);
    const auto  members_eq      = std::ranges::equal(
        members_, casted.members_, [](const auto& a, const auto& b) { return *a == *b; });
    return mem::nullable_boxes_eq(underlying_, casted.underlying_) && enumerations_eq && members_eq;
}

} // namespace porpoise::ast

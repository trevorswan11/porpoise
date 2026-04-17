#include <algorithm>

#include "ast/expressions/union.hpp"

#include "ast/expressions/enum.hpp"
#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/struct.hpp" // IWYU pragma: keep
#include "ast/expressions/type.hpp"
#include "ast/expressions/union.hpp"
#include "ast/statements/declaration.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

UnionField::UnionField(mem::Box<IdentifierExpression> ident, ExplicitType&& type) noexcept
    : ident_{std::move(ident)}, type_{std::move(type)} {}
UnionField::~UnionField() = default;

auto UnionField::accept(Visitor& v) const -> void { v.visit(*this); }

[[nodiscard]] auto UnionField::get_token() const noexcept -> const syntax::Token& {
    return ident_->get_token();
}

auto UnionField::is_equal(const UnionField& other) const noexcept -> bool {
    return *ident_ == *other.ident_ && type_ == other.type_;
}

UnionExpression::UnionExpression(const syntax::Token& start_token,
                                 Fields               fields,
                                 Members              members) noexcept
    : ExprBase{start_token}, fields_{std::move(fields)}, members_{std::move(members)} {}
UnionExpression::~UnionExpression() = default;

auto UnionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnionExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));

    Fields fields;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.get_peek_token().is_decl_token()) { break; }

        TRY(parser.expect_peek(syntax::TokenType::IDENT));
        auto ident = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

        TRY(parser.expect_peek(syntax::TokenType::COLON));
        auto type = TRY(ExplicitType::parse(parser));

        fields.emplace_back(std::move(ident), std::move(type));

        // No comma means that its the end or that there is a decl list starting
        if (!parser.peek_token_is(syntax::TokenType::COMMA)) { break; }
        parser.advance();
    }

    auto members = TRY(parser.parse_member_decls(validate_non_struct_member));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    // Validate here so that there aren't 3 errors spawning from an empty union with decls
    if (fields.empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_UNION, start_token);
    }
    return mem::make_box<UnionExpression>(start_token, std::move(fields), std::move(members));
}

auto UnionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<UnionExpression>(other);
    return std::ranges::equal(fields_, casted.fields_) &&
           std::ranges::equal(
               members_, casted.members_, [](const auto& a, const auto& b) { return *a == *b; });
}

} // namespace porpoise::ast

#include <algorithm>

#include "ast/expressions/initializer.hpp"

#include "ast/expressions/prefix.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

Initializer::Initializer(mem::Box<ImplicitAccessExpression> member,
                         mem::Box<Expression>               value) noexcept
    : member_{std::move(member)}, value_{std::move(value)} {}
Initializer::~Initializer() = default;

auto Initializer::accept(Visitor& v) const -> void { v.visit(*this); }

auto Initializer::is_equal(const Initializer& other) const noexcept -> bool {
    return *member_ == *other.member_ && *value_ == *other.value_;
}

InitializerExpression::InitializerExpression(const syntax::Token&           start_token,
                                             Optional<mem::Box<Expression>> object,
                                             std::vector<Initializer>       initializers) noexcept
    : ExprBase{start_token}, object_{std::move(object)}, initializers_{std::move(initializers)} {}
InitializerExpression::~InitializerExpression() = default;

auto InitializerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto InitializerExpression::parse_opt_object(syntax::Parser&                parser,
                                             Optional<mem::Box<Expression>> object)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    std::vector<Initializer> initializers;
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        TRY(parser.expect_peek(syntax::TokenType::DOT));
        auto member =
            downcast<ImplicitAccessExpression>(TRY(ImplicitAccessExpression::parse(parser)));

        TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
        parser.advance();
        auto value = TRY(parser.parse_expression());
        initializers.emplace_back(std::move(member), std::move(value));

        if (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));

    return mem::make_box<InitializerExpression>(
        start_token, std::move(object), std::move(initializers));
}

auto InitializerExpression::parse(syntax::Parser& parser, mem::Box<Expression> object)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    return parse_opt_object(parser, std::move(object));
}

auto InitializerExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<InitializerExpression>(other);
    return optional::unsafe_eq<Expression>(object_, casted.object_) &&
           std::ranges::equal(initializers_, casted.initializers_);
}

} // namespace porpoise::ast

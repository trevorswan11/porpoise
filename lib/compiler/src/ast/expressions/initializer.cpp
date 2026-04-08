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
                                             Optional<mem::Box<Expression>> object_type,
                                             std::vector<Initializer>       initializers) noexcept
    : ExprBase{start_token}, object_type_{std::move(object_type)},
      initializers_{std::move(initializers)} {}
InitializerExpression::~InitializerExpression() = default;

auto InitializerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto InitializerExpression::parse_opt_object(syntax::Parser&                parser,
                                             Optional<mem::Box<Expression>> object_type)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = object_type.transform([](const auto& obj) { return obj->get_token(); })
                                 .value_or(parser.current_token());

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
        start_token, std::move(object_type), std::move(initializers));
}

auto InitializerExpression::parse(syntax::Parser& parser, mem::Box<Expression> object_type)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    return parse_opt_object(parser, std::move(object_type));
}

auto InitializerExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<InitializerExpression>(other);
    return optional::unsafe_eq<Expression>(object_type_, casted.object_type_) &&
           std::ranges::equal(initializers_, casted.initializers_);
}

} // namespace porpoise::ast

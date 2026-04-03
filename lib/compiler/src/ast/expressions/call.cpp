#include <algorithm>

#include "ast/expressions/call.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp" // IWYU pragma: keep
#include "ast/expressions/type.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

CallArgument::CallArgument(mem::Box<Expression> argument) noexcept
    : argument_{std::move(argument)} {}
CallArgument::CallArgument(ExplicitType&& argument) noexcept : argument_{std::move(argument)} {}
CallArgument::~CallArgument() = default;

auto CallArgument::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallArgument::is_equal(const CallArgument& other) const noexcept -> bool {
    const auto& other_argument = other.argument_;
    if (other.argument_.index() != other_argument.index()) { return false; }
    return std::visit(Overloaded{[&other_argument](const mem::Box<Expression>& e) {
                                     return *e == *std::get<mem::Box<Expression>>(other_argument);
                                 },
                                 [&other_argument](const ExplicitType& e) {
                                     return e == std::get<ExplicitType>(other_argument);
                                 }},
                      argument_);
}

CallExpression::CallExpression(const syntax::Token&      start_token,
                               mem::Box<Expression>      function,
                               std::vector<CallArgument> arguments) noexcept
    : ExprBase{start_token}, function_{std::move(function)}, arguments_{std::move(arguments)} {}
CallExpression::~CallExpression() = default;

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(syntax::Parser& parser, mem::Box<Expression> function)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    std::vector<CallArgument> arguments;
    // Guaranteed to roll back if there is an error
    const auto parse_expr_unsuccessful = [&]() {
        // Try an expression first to prevent ambiguity between reference operators
        syntax::Parser::Transaction transaction{parser};
        parser.advance();
        if (auto expr = parser.parse_expression()) {
            transaction.commit();
            arguments.emplace_back(std::move(*expr));
            return false;
        }
        return true;
    };

    while (!parser.peek_token_is(syntax::TokenType::RPAREN) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        if (parser.peek_token_is(syntax::TokenType::COMMA)) {
            return make_parser_unexpected(syntax::ParserError::COMMA_WITH_MISSING_CALL_ARGUMENT,
                                          parser.peek_token());
        }

        // Advance cannot be called here since explicit type relies on peek, not current
        if (parse_expr_unsuccessful()) { arguments.emplace_back(TRY(ExplicitType::parse(parser))); }
        if (!parser.peek_token_is(syntax::TokenType::RPAREN)) {
            TRY(parser.expect_peek(syntax::TokenType::COMMA));
        }
    }
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    return mem::make_box<CallExpression>(
        function->get_token(), std::move(function), std::move(arguments));
}

auto CallExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<CallExpression>(other);
    return *function_ == *casted.function_ && std::ranges::equal(arguments_, casted.arguments_);
}

} // namespace porpoise::ast

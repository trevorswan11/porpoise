#include <algorithm>
#include <utility>

#include "ast/expressions/match.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

MatchArm::MatchArm(mem::Box<Expression> pattern,
                   Optional<Capture>    capture,
                   mem::Box<Statement>  dispatch) noexcept
    : pattern_{std::move(pattern)}, capture_{std::move(capture)}, dispatch_{std::move(dispatch)} {}
MatchArm::~MatchArm() = default;

auto MatchArm::accept(Visitor& v) const -> void { v.visit(*this); }

auto MatchArm::is_equal(const MatchArm& other) const noexcept -> bool {
    return *pattern_ == *other.pattern_ &&
           optional::safe_eq<Capture>(
               capture_,
               other.capture_,
               [](const Capture& a, const Capture& b) {
                   if (a.index() != b.index()) { return false; }
                   return std::visit(
                       Overloaded{[&a](const mem::Box<IdentifierExpression>& b) {
                                      return *b == *std::get<mem::Box<IdentifierExpression>>(a);
                                  },
                                  [](const unit&) { return true; }},
                       b);
               }) &&
           *dispatch_ == *other.dispatch_;
}

MatchExpression::MatchExpression(const syntax::Token&        start_token,
                                 mem::Box<Expression>        matcher,
                                 std::vector<MatchArm>       arms,
                                 mem::NullableBox<Statement> catch_all) noexcept
    : ExprBase{start_token}, matcher_{std::move(matcher)}, arms_{std::move(arms)},
      catch_all_{std::move(catch_all)} {}
MatchExpression::~MatchExpression() = default;

auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto MatchExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(syntax::TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(syntax::TokenType::RPAREN)) {
        return make_parser_unexpected(syntax::ParserError::MATCH_EXPR_MISSING_CONDITION,
                                      start_token);
    }

    auto matcher = TRY(parser.parse_expression());
    TRY(parser.expect_peek(syntax::TokenType::RPAREN));

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    if (parser.peek_token_is(syntax::TokenType::RBRACE)) {
        parser.advance();
        return make_parser_unexpected(syntax::ParserError::ARMLESS_MATCH_EXPR, start_token);
    }

    std::vector<MatchArm> arms;
    // Current token is either the LBRACE at the start or a comma before parsing
    while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
           !parser.peek_token_is(syntax::TokenType::END)) {
        parser.advance();

        auto pattern = TRY(parser.parse_expression());
        TRY(parser.expect_peek(syntax::TokenType::FAT_ARROW));

        // There is an optional capture for every arm
        Optional<MatchArm::Capture> capture;
        if (parser.peek_token_is(syntax::TokenType::BW_OR)) {
            parser.advance();

            // An underscore is equivalent to a lack of capture
            if (parser.peek_token_is(syntax::TokenType::UNDERSCORE)) {
                parser.advance();
                capture.emplace(unit{});
            } else {
                TRY(parser.expect_peek(syntax::TokenType::IDENT));
                capture.emplace(
                    downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser))));
            }
            TRY(parser.expect_peek(syntax::TokenType::BW_OR));
        }

        // The resulting statement must be restricted like an if branch
        parser.advance();
        auto consequence =
            TRY(parser.parse_restricted_statement(syntax::ParserError::ILLEGAL_MATCH_ARM));
        arms.emplace_back(std::move(pattern), std::move(capture), std::move(consequence));
    }
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    auto catch_all =
        TRY(parser.try_parse_restricted_alternate(syntax::ParserError::ILLEGAL_MATCH_CATCH_ALL));

    return mem::make_box<MatchExpression>(
        start_token, std::move(matcher), std::move(arms), std::move(catch_all));
}

auto MatchExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted  = as<MatchExpression>(other);
    const auto  arms_eq = std::ranges::equal(arms_, casted.arms_);
    return *matcher_ == *casted.matcher_ && arms_eq &&
           mem::nullable_boxes_eq(catch_all_, casted.catch_all_);
}

} // namespace porpoise::ast

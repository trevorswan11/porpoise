#include <algorithm>

#include "ast/expressions/struct.hpp"

#include "ast/statements/declaration.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

StructExpression::StructExpression(const syntax::Token&            start_token,
                                   std::vector<Box<DeclStatement>> members) noexcept
    : ExprBase{start_token}, members_{std::move(members)} {}
StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(syntax::Parser& parser)
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (parser.current_token_is(syntax::TokenType::PACKED)) {
        TRY(parser.expect_peek(syntax::TokenType::STRUCT));
    } else if (parser.peek_token_is(syntax::TokenType::PACKED)) {
        return make_parser_unexpected(syntax::ParserError::PACKED_AFTER_STRUCT_KEYWORD,
                                      start_token);
    }

    std::vector<Box<DeclStatement>> members;
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    while (!parser.peek_token_is(syntax::TokenType::RBRACE)) {
        parser.advance();
        auto member = TRY(parser.parse_statement());
        if (!member->is<DeclStatement>()) {
            return make_parser_unexpected(syntax::ParserError::INVALID_STRUCT_MEMBER,
                                          member->get_token());
        }

        members.emplace_back(downcast<DeclStatement>(std::move(member)));
    }

    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    if (members.empty()) {
        return make_parser_unexpected(syntax::ParserError::EMPTY_STRUCT, start_token);
    }
    return make_box<StructExpression>(start_token, std::move(members));
}

auto StructExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted     = as<StructExpression>(other);
    const auto  members_eq = std::ranges::equal(
        members_, casted.members_, [](const auto& a, const auto& b) { return *a == *b; });

    return is_packed() == casted.is_packed() && members_eq;
}

} // namespace porpoise::ast

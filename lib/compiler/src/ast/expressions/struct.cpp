#include "ast/expressions/struct.hpp"

#include "ast/statements/declaration.hpp" // IWYU pragma: keep
#include "ast/visitor.hpp"

namespace porpoise::ast {

StructExpression::StructExpression(const syntax::Token& start_token, Members members) noexcept
    : ExprBase{start_token}, members_{std::move(members)} {}
StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    if (parser.current_token_is(syntax::TokenType::PACKED)) {
        TRY(parser.expect_peek(syntax::TokenType::STRUCT));
    } else if (parser.peek_token_is(syntax::TokenType::PACKED)) {
        return make_parser_err(syntax::ParserError::PACKED_AFTER_STRUCT_KEYWORD, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto members = TRY(Members::parse(parser,
                                      Overloaded{[](const mem::Box<DeclStatement>& decl) {
                                                     return Members::validate_struct_decl(*decl);
                                                 },
                                                 [](const auto&) { return true; }}));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    if (members.empty()) { return make_parser_err(syntax::ParserError::EMPTY_STRUCT, start_token); }
    return mem::make_box<StructExpression>(start_token, std::move(members));
}

auto StructExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<StructExpression>(other);
    return is_packed() == casted.is_packed() && members_ == casted.members_;
}

} // namespace porpoise::ast

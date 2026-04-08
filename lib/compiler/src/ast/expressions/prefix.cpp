#include "ast/expressions/prefix.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/initializer.hpp"

namespace porpoise::ast {

// cppcheck-suppress-begin duplInheritedMember

auto ImplicitAccessExpression::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic> {
    // We need to explicitly jump into the initializer expression here
    if (parser.peek_token_is(syntax::TokenType::LBRACE)) {
        parser.advance();
        return InitializerExpression::parse_opt_object(parser);
    }

    // Otherwise it suffices to fall back to standard prefix parsing
    const auto prefix_token = parser.current_token();
    if (parser.peek_token_is(syntax::TokenType::END)) {
        return make_parser_unexpected(syntax::ParserError::PREFIX_MISSING_OPERAND, prefix_token);
    }

    parser.advance();
    auto operand = TRY(parser.parse_expression(syntax::Precedence::PREFIX));
    if (!operand->is<IdentifierExpression>()) {
        return make_parser_unexpected(syntax::ParserError::ILLEGAL_IMPLICIT_ACCESS_OPERAND,
                                      operand->get_token());
    }

    return mem::make_box<ImplicitAccessExpression>(prefix_token, std::move(operand));
}

// cppcheck-suppress-end duplInheritedMember

} // namespace porpoise::ast

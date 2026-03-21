#include "ast/expressions/identifier.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(
    syntax::Parser& parser) // cppcheck-suppress constParameterReference
    -> Expected<Box<Expression>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (!start_token.is_valid_ident()) {
        return make_parser_unexpected(syntax::ParserError::ILLEGAL_IDENTIFIER, start_token);
    }

    return make_box<IdentifierExpression>(start_token);
}

} // namespace porpoise::ast

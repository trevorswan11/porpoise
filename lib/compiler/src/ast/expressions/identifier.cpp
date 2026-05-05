#include "ast/expressions/identifier.hpp"

#include "ast/visitor.hpp"

namespace porpoise::ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(
    syntax::Parser& parser) // cppcheck-suppress constParameterReference
    -> Result<mem::Box<Expression>, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    if (!start_token.is_valid_ident()) {
        return make_syntax_err(syntax::Error::ILLEGAL_IDENTIFIER, start_token);
    }

    return mem::make_box<IdentifierExpression>(start_token);
}

} // namespace porpoise::ast

#include "helpers/ast.hpp"

namespace porpoise::tests::helpers {

auto ident_from(std::string_view name) -> ast::IdentifierExpression {
    const auto extract = [](const auto& key) { return key.second; };
    const auto tt      = syntax::get_keyword(name).transform(extract).value_or(
        syntax::get_builtin(name).transform(extract).value_or(syntax::TokenType::IDENT));
    return ast::IdentifierExpression{syntax::Token{tt, name}};
}

auto ident_from(const syntax::Token& tok) -> ast::IdentifierExpression {
    return ast::IdentifierExpression{tok};
}

} // namespace porpoise::tests::helpers

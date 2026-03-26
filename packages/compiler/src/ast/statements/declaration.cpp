#include "ast/statements/declaration.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

DeclStatement::DeclStatement(const syntax::Token&           start_token,
                             mem::Box<IdentifierExpression> ident,
                             mem::Box<TypeExpression>       type,
                             Optional<mem::Box<Expression>> value,
                             DeclModifiers                  modifiers) noexcept
    : StmtBase{start_token}, ident_{std::move(ident)}, type_{std::move(type)},
      value_{std::move(value)}, modifiers_{modifiers} {}
DeclStatement::~DeclStatement() = default;

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeclStatement::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    auto       modifiers   = token_to_modifier(start_token).value();

    Optional<DeclModifiers> current_modifier;
    while ((current_modifier = token_to_modifier(parser.peek_token()))) {
        parser.advance();
        if (modifiers_has(modifiers, *current_modifier)) {
            return make_parser_unexpected(syntax::ParserError::DUPLICATE_DECL_MODIFIER,
                                          start_token);
        }
        modifiers |= *current_modifier;
    }

    if (!validate_modifiers(modifiers)) {
        return make_parser_unexpected(syntax::ParserError::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    auto decl_name = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));
    auto decl_type_expr                 = downcast<TypeExpression>(std::move(decl_type));

    Optional<mem::Box<Expression>> decl_value;
    if (value_initialized) {
        decl_value = TRY(parser.parse_expression());
        // If there is a value, then there cannot be an extern keyword
        if (modifiers_has(modifiers, DeclModifiers::EXTERN)) {
            return make_parser_unexpected(syntax::ParserError::EXTERN_VALUE_INITIALIZED,
                                          start_token);
        }
    } else if ((modifiers_has(modifiers, DeclModifiers::CONSTANT) &&
                !modifiers_has(modifiers, DeclModifiers::EXTERN)) ||
               modifiers_has(modifiers, DeclModifiers::COMPTIME)) {
        // Constant decls must be declared with a value unless they are extern
        return make_parser_unexpected(syntax::ParserError::CONST_DECL_MISSING_VALUE, start_token);
    }

    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {
        TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    }
    return mem::make_box<DeclStatement>(start_token,
                                        std::move(decl_name),
                                        std::move(decl_type_expr),
                                        std::move(decl_value),
                                        modifiers);
}

auto DeclStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<DeclStatement>(other);
    return *ident_ == *casted.ident_ && *type_ == *casted.type_ &&
           optional::unsafe_eq<Expression>(value_, casted.value_) &&
           modifiers_ == casted.modifiers_;
}

} // namespace porpoise::ast

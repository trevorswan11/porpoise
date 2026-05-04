#include <algorithm>
#include <array>
#include <bit>

#include "ast/statements/declaration.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

namespace {

using ModifierMapping          = std::pair<syntax::TokenType, DeclModifiers>;
constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
    {syntax::TokenType::VAR, DeclModifiers::VARIABLE},
    {syntax::TokenType::CONSTANT, DeclModifiers::CONSTANT},
    {syntax::TokenType::CONSTEXPR, DeclModifiers::CONSTEXPR},
    {syntax::TokenType::PUBLIC, DeclModifiers::PUBLIC},
    {syntax::TokenType::EXTERN, DeclModifiers::EXTERN},
    {syntax::TokenType::EXPORT, DeclModifiers::EXPORT},
    {syntax::TokenType::STATIC, DeclModifiers::STATIC},
});

[[nodiscard]] constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept -> bool {
    // Exactly one mutability flag must be set
    const auto valid_mut = std::popcount(std::to_underlying(
                               modifiers & (DeclModifiers::VARIABLE | DeclModifiers::CONSTANT |
                                            DeclModifiers::CONSTEXPR))) == 1;

    // Comptime values cannot be known at link time, obviously
    const auto valid_constexpr =
        std::popcount(std::to_underlying(modifiers &
                                         (DeclModifiers::EXTERN | DeclModifiers::CONSTEXPR))) <= 1;

    // At most one ABI flag can be set
    const auto valid_abi = std::popcount(std::to_underlying(
                               modifiers & (DeclModifiers::EXTERN | DeclModifiers::EXPORT))) <= 1;
    return valid_mut && valid_constexpr && valid_abi;
}

[[nodiscard]] constexpr auto token_to_modifier(const syntax::Token& tok)
    -> opt::Option<DeclModifiers> {
    const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
    return it == LEGAL_MODIFIERS.end() ? opt::none : opt::Option<DeclModifiers>{it->second};
}

} // namespace

DeclStatement::DeclStatement(const syntax::Token&           start_token,
                             mem::Box<IdentifierExpression> ident,
                             mem::Box<TypeExpression>       type,
                             mem::NullableBox<Expression>   value,
                             DeclModifiers                  modifiers) noexcept
    : StmtBase{start_token}, ident_{std::move(ident)}, type_{std::move(type)},
      value_{std::move(value)}, modifiers_{modifiers} {}
DeclStatement::~DeclStatement() = default;

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeclStatement::parse(syntax::Parser& parser)
    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    auto       modifiers   = token_to_modifier(start_token).value();

    opt::Option<DeclModifiers> current_modifier;
    while ((current_modifier = token_to_modifier(parser.get_peek_token()))) {
        parser.advance();
        if (modifiers_has(modifiers, *current_modifier)) {
            return make_parser_err(syntax::ParserError::DUPLICATE_DECL_MODIFIER, start_token);
        }
        modifiers |= *current_modifier;
    }

    if (!validate_modifiers(modifiers)) {
        return make_parser_err(syntax::ParserError::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    auto decl_name = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));
    auto decl_type_expr                 = downcast<TypeExpression>(std::move(decl_type));

    mem::NullableBox<Expression> decl_value;
    if (value_initialized) {
        decl_value = mem::nullable_box_from(TRY(parser.parse_expression()));

        // If there is a value, then there cannot be an extern keyword
        if (modifiers_has(modifiers, DeclModifiers::EXTERN)) {
            return make_parser_err(syntax::ParserError::EXTERN_VALUE_INITIALIZED, start_token);
        }
    } else if ((modifiers_has(modifiers, DeclModifiers::CONSTANT) &&
                !modifiers_has(modifiers, DeclModifiers::EXTERN)) ||
               modifiers_has(modifiers, DeclModifiers::CONSTEXPR)) {
        // Constant decls must be declared with a value unless they are extern
        return make_parser_err(syntax::ParserError::CONST_DECL_MISSING_VALUE, start_token);
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
           mem::nullable_boxes_eq(value_, casted.value_) && modifiers_ == casted.modifiers_;
}

} // namespace porpoise::ast

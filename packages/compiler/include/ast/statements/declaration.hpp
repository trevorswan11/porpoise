#pragma once

#include <algorithm>
#include <array>
#include <bit>

#include <magic_enum/magic_enum_flags.hpp>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class TypeExpression;

enum class DeclModifiers : u8 {
    VARIABLE = 1 << 0,
    CONSTANT = 1 << 1,
    COMPTIME = 1 << 2,
    PUBLIC   = 1 << 3,
    EXTERN   = 1 << 4,
    EXPORT   = 1 << 5,
    STATIC   = 1 << 6,
};

constexpr auto operator|(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr auto operator&(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr auto operator^(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

constexpr auto operator|=(DeclModifiers& lhs, DeclModifiers rhs) -> DeclModifiers& {
    lhs = lhs | rhs;
    return lhs;
}

class DeclStatement : public StmtBase<DeclStatement> {
  public:
    static constexpr auto KIND = NodeKind::DECL_STATEMENT;

  public:
    explicit DeclStatement(const syntax::Token&           start_token,
                           mem::Box<IdentifierExpression> ident,
                           mem::Box<TypeExpression>       type,
                           Optional<mem::Box<Expression>> value,
                           DeclModifiers                  modifiers) noexcept;
    ~DeclStatement() override;

    MAKE_AST_COPY_MOVE(DeclStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_GETTER(type, const TypeExpression&, *)
    MAKE_OPTIONAL_UNPACKER(value, Expression, value_, **)
    MAKE_GETTER(modifiers, const DeclModifiers&)

    [[nodiscard]] auto has_modifier(DeclModifiers flag) const noexcept -> bool {
        return modifiers_has(modifiers_, flag);
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    static auto modifiers_has(DeclModifiers modifiers, DeclModifiers flag) noexcept -> bool {
        return static_cast<bool>(modifiers & flag);
    }

  private:
    using ModifierMapping                 = std::pair<syntax::TokenType, DeclModifiers>;
    static constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
        {syntax::TokenType::VAR, DeclModifiers::VARIABLE},
        {syntax::TokenType::CONST, DeclModifiers::CONSTANT},
        {syntax::TokenType::COMPTIME, DeclModifiers::COMPTIME},
        {syntax::TokenType::PUBLIC, DeclModifiers::PUBLIC},
        {syntax::TokenType::EXTERN, DeclModifiers::EXTERN},
        {syntax::TokenType::EXPORT, DeclModifiers::EXPORT},
        {syntax::TokenType::STATIC, DeclModifiers::STATIC},
    });

    static constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept -> bool {
        // Exactly one mutability flag must be set
        const auto valid_mut = std::popcount(std::to_underlying(
                                   modifiers & (DeclModifiers::VARIABLE | DeclModifiers::CONSTANT |
                                                DeclModifiers::COMPTIME))) == 1;

        // Comptime values cannot be known at link time, obviously
        const auto valid_comptime =
            std::popcount(std::to_underlying(
                modifiers & (DeclModifiers::EXTERN | DeclModifiers::COMPTIME))) <= 1;

        // At most one ABI flag can be set
        const auto valid_abi =
            std::popcount(std::to_underlying(modifiers &
                                             (DeclModifiers::EXTERN | DeclModifiers::EXPORT))) <= 1;
        return valid_mut && valid_comptime && valid_abi;
    }

    static constexpr auto token_to_modifier(const syntax::Token& tok) -> Optional<DeclModifiers> {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? std::nullopt : Optional<DeclModifiers>{it->second};
    }

  private:
    mem::Box<IdentifierExpression> ident_;
    mem::Box<TypeExpression>       type_;
    Optional<mem::Box<Expression>> value_;
    DeclModifiers                  modifiers_;
};

} // namespace porpoise::ast

template <> struct magic_enum::customize::enum_range<porpoise::ast::DeclModifiers> {
    static constexpr bool is_flags = true;
};

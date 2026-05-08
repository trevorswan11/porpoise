#pragma once

#include <magic_enum/magic_enum_flags.hpp>

#include "ast/node.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

class IdentifierExpression;
class TypeExpression;

enum class DeclModifiers : u8 {
    VARIABLE  = 1 << 0,
    CONSTANT  = 1 << 1,
    CONSTEXPR = 1 << 2,
    PUBLIC    = 1 << 3,
    EXTERN    = 1 << 4,
    EXPORT    = 1 << 5,
    STATIC    = 1 << 6,
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
    DeclStatement(const syntax::Token&           start_token,
                  mem::Box<IdentifierExpression> ident,
                  mem::Box<TypeExpression>       type,
                  mem::NullableBox<Expression>   value,
                  DeclModifiers                  modifiers) noexcept;
    ~DeclStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(DeclStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::Diagnostic>;

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_GETTER(type, const TypeExpression&, *)
    MAKE_NULLABLE_BOX_UNPACKER(value, Expression, value_, *)
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
    mem::Box<IdentifierExpression> ident_;
    mem::Box<TypeExpression>       type_;
    mem::NullableBox<Expression>   value_;
    DeclModifiers                  modifiers_;
};

} // namespace porpoise::ast

template <> struct magic_enum::customize::enum_range<porpoise::ast::DeclModifiers> {
    static constexpr bool is_flags = true;
};

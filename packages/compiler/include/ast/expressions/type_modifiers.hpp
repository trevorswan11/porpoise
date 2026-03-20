#pragma once

#include <algorithm>
#include <utility>

#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "lexer/token.hpp"

#include "optional.hpp"
#include "types.hpp"

namespace porpoise::ast {

#define MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(name, modifier)  \
    [[nodiscard]] auto is_##name() const noexcept -> bool { \
        if (is_value()) { return false; }                   \
        return *underlying_ == modifier;                    \
    }

class TypeModifier {
  public:
    enum class Modifier : u8 {
        REF,
        MUT_REF,
        PTR,
        MUT_PTR,
    };

  public:
    TypeModifier() noexcept = default;
    explicit TypeModifier(Optional<Modifier> underlying) noexcept
        : underlying_{std::move(underlying)} {}

    static constexpr auto from_token(const Token& tok) noexcept -> TypeModifier {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? TypeModifier{std::nullopt} : TypeModifier{it->second};
    }

    // Whether or not the type is a 'value' type (no modifier), mutually exclusive result.
    [[nodiscard]] auto is_value() const noexcept -> bool { return !underlying_; }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ref, Modifier::MUT_REF)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ref, Modifier::REF)
    [[nodiscard]] auto is_ref() const noexcept -> bool {
        return is_mutable_ref() || is_const_ref();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ptr, Modifier::MUT_PTR)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ptr, Modifier::PTR)
    [[nodiscard]] auto is_ptr() const noexcept -> bool {
        return is_mutable_ptr() || is_const_ptr();
    }

    friend auto operator==(const TypeModifier& lhs, const TypeModifier& rhs) noexcept -> bool {
        return lhs.underlying_ == rhs.underlying_;
    }

  private:
    using ModifierMapping                 = std::pair<TokenType, Modifier>;
    static constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
        {TokenType::BW_AND, Modifier::REF},
        {TokenType::AND_MUT, Modifier::MUT_REF},
        {TokenType::STAR, Modifier::PTR},
        {TokenType::STAR_MUT, Modifier::MUT_PTR},
    });

  private:
    Optional<Modifier> underlying_;

    friend struct fmt::formatter<porpoise::ast::TypeModifier>;
};

#undef MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY

} // namespace porpoise::ast

template <> struct fmt::formatter<porpoise::ast::TypeModifier> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::TypeModifier& t, F& ctx) {
        return fmt::format_to(
            ctx.out(),
            "{}",
            t.underlying_.transform([](const auto& u) { return magic_enum::enum_name(u); })
                .value_or("BASE"));
    }
};

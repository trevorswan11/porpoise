#pragma once

#include <utility>

#include <fmt/format.h>

#include <magic_enum/magic_enum.hpp>

#include "syntax/token.hpp"

#include "option.hpp"
#include "types.hpp"

namespace porpoise {

namespace ast {

#define MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(name, modifier)  \
    [[nodiscard]] auto is_##name() const noexcept -> bool { \
        if (is_value()) { return false; }                   \
        return *underlying_ == modifier;                    \
    }

namespace detail {

enum class Modifier : u8 {
    REF,
    MUT_REF,
    PTR,
    MUT_PTR,
    VOLATILE,
};

} // namespace detail

} // namespace ast

namespace opt {

template <> struct SentinelEnum<ast::detail::Modifier> {
    static constexpr auto SENTINEL = EnumLimits<ast::detail::Modifier>::max();
};

} // namespace opt

namespace ast {

class TypeModifier {
  public:
    using Modifier = detail::Modifier;

  public:
    TypeModifier() noexcept = default;
    explicit TypeModifier(opt::Enum<Modifier> underlying) noexcept
        : underlying_{std::move(underlying)} {}
    explicit TypeModifier(const syntax::Token& tok) noexcept;

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

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(volatile, Modifier::VOLATILE)

    friend auto operator==(const TypeModifier& lhs, const TypeModifier& rhs) noexcept -> bool {
        return lhs.underlying_ == rhs.underlying_;
    }

  private:
    opt::Enum<Modifier> underlying_;

    friend struct fmt::formatter<porpoise::ast::TypeModifier>;
};

#undef MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY

} // namespace ast

} // namespace porpoise

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

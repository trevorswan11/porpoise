#pragma once

#include <fmt/format.h>

#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/primitive.hh"

template <> struct fmt::formatter<porpoise::ast::TypeModifier> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::TypeModifier& t, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(t.underlying_));
    }
};

template <> struct fmt::formatter<porpoise::ast::IdentifierExpression> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::IdentifierExpression& n, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", n.name);
    }
};

template <porpoise::ast::traits::ValuedPrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.value);
    }
};

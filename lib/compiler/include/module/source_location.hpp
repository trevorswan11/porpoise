#pragma once

#include <concepts>
#include <utility>

#include <fmt/format.h>

#include "option.hpp"
#include "types.hpp"

namespace porpoise {

namespace mod { struct Module; } // namespace mod

struct SourceLocation {
    usize                     line   = 0;
    usize                     column = 0;
    opt::Option<mod::Module&> mod;

    SourceLocation() noexcept = default;
    SourceLocation(usize line, usize column) noexcept : line{line}, column{column} {}

    auto operator==(const SourceLocation& other) const noexcept -> bool {
        return line == other.line && column == other.column;
    }
};

template <typename T> struct SourceInfo;

template <typename T>
concept Locateable = requires(T t) {
    { SourceInfo<T>::get(t) } -> std::same_as<SourceLocation>;
};

template <> struct SourceInfo<std::pair<usize, usize>> {
    static auto get(const std::pair<usize, usize>& p) noexcept -> SourceLocation {
        return {p.first, p.second};
    }
};

} // namespace porpoise

template <> struct fmt::formatter<porpoise::SourceLocation> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::SourceLocation& loc, F& ctx) {
        return fmt::format_to(ctx.out(), "[{}, {}]", loc.line, loc.column);
    }
};

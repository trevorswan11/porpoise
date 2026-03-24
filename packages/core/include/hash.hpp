#pragma once

#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "types.hpp"

namespace porpoise::hash {

namespace wyhash = ankerl::unordered_dense::detail::wyhash;

template <typename T> [[nodiscard]] auto hash(const T& value) noexcept -> u64 {
    if constexpr (std::is_convertible_v<T, u64>) { return wyhash::hash(static_cast<u64>((value))); }
    return ankerl::unordered_dense::hash<T>{}(value);
}

template <typename T> auto combine(u64& h, const T& value) noexcept -> void {
    h = wyhash::mix(h, hash(value));
}

} // namespace porpoise::hash

#pragma once

#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "types.hpp"

namespace porpoise::hash {

namespace wyhash = ankerl::unordered_dense::detail::wyhash;

class Hasher {
  public:
    template <typename T> explicit Hasher(const T& initial) : hash_{hash<T>(initial)} {}

    template <typename T> auto combine(const T& value) noexcept -> void {
        hash_ = wyhash::mix(hash_, hash(value));
    }

    [[nodiscard]] auto finalize() const noexcept -> u64 { return hash_; }

  private:
    template <typename T> [[nodiscard]] static auto hash(const T& value) noexcept -> u64 {
        if constexpr (std::is_convertible_v<T, u64>) {
            return wyhash::hash(static_cast<u64>(value));
        }
        return ankerl::unordered_dense::hash<T>{}(value);
    }

  private:
    u64 hash_;
};

} // namespace porpoise::hash

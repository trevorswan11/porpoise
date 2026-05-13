#pragma once

#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "types.hh"

namespace porpoise::hash {

template <typename T>
concept Wyhashable = std::is_convertible_v<T, u64>;

namespace wyhash = ankerl::unordered_dense::detail::wyhash;

template <typename T> struct Hash {
    [[nodiscard]] static auto operator()(const T& t) noexcept -> u64 {
        if constexpr (Wyhashable<T>) { return wyhash::hash(static_cast<u64>(t)); }
        return ankerl::unordered_dense::hash<T>{}(t);
    }
};

// A 'high-quality' hash backed by `wyhash` with a `std::hash` fallback
class Hasher {
  public:
    // Hashes the provided value to use as the initial hashed value
    template <typename T> constexpr explicit Hasher(const T& initial) : hash_{hash(initial)} {}
    constexpr Hasher() noexcept : hash_{0} {}

    // Hashes the provided value and mixes the result with the current hash
    template <typename T> constexpr auto combine(const T& value) noexcept -> void {
        hash_ = wyhash::mix(hash_, hash(value));
    }

    template <> constexpr auto combine<Hasher>(const Hasher& value) noexcept -> void {
        hash_ = wyhash::mix(hash_, value.finalize());
    }

    // Call this after a full operation to get the resulting hash
    [[nodiscard]] constexpr auto finalize() const noexcept -> u64 { return hash_; }
    [[nodiscard]] constexpr bool operator==(const Hasher&) const noexcept = default;

  private:
    template <typename T> [[nodiscard]] static constexpr auto hash(const T& value) noexcept -> u64 {
        return Hash<T>{}(value);
    }

  private:
    u64 hash_;
};

// https://github.com/martinus/unordered_dense#324-heterogeneous-overloads-using-is_transparent
struct StringTransparent {
    using is_transparent = void;
    using is_avalanching = void;

    [[nodiscard]] static auto operator()(std::string_view str) noexcept -> u64 {
        return ankerl::unordered_dense::hash<std::string_view>{}(str);
    }
};

} // namespace porpoise::hash

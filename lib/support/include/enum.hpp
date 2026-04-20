#pragma once

#include <array>
#include <concepts>
#include <type_traits>
#include <utility>

#include <magic_enum/magic_enum.hpp>

#include "iterator.hpp"
#include "memory.hpp"
#include "optional.hpp"
#include "types.hpp"

namespace porpoise {

template <typename Enum>
concept ValidEnum = std::is_enum_v<Enum>;

using magic_enum::enum_value;

template <ValidEnum E> consteval auto enum_min_value() { return enum_value<E>(0); }
template <ValidEnum E> consteval auto enum_min_underlying() {
    return magic_enum::enum_integer(enum_min_value<E>());
}

template <ValidEnum E> consteval auto enum_max_value() {
    return enum_value<E>(magic_enum::enum_count<E>() - 1);
}

template <ValidEnum E> consteval auto enum_max_underlying() {
    return magic_enum::enum_integer(enum_max_value<E>());
}

// Requires that the enum is in an exclusively bounded range of magic enum bounds
//
// This prevents possible bugs and should be fixed by adjusting defines in `build.zig`
template <typename Enum>
concept BoundedEnum = ValidEnum<Enum> && requires {
    enum_min_underlying<Enum>() > MAGIC_ENUM_RANGE_MIN &&
        enum_max_underlying<Enum>() < MAGIC_ENUM_RANGE_MAX;
};

// Empty or single-value enums aren't allowed since they eliminate the need for mapping
template <typename Enum>
concept MappableEnum = BoundedEnum<Enum> && magic_enum::enum_count<Enum>() > 1;

// Box's aren't allowed in enum maps as the maps are meant for compile-time generation
template <typename T>
concept MappableValue =
    !mem::is_box_v<std::remove_cvref_t<T>> && !mem::is_nullable_box_v<std::remove_cvref_t<T>>;

// An O(1) map that stores optional values for each enumeration
template <MappableEnum E, MappableValue Value> class EnumMap {
  public:
    using Map = std::array<Value, magic_enum::enum_count<E>()>;
    MAKE_UNALIASED_ITERATOR(Map, map_)

  public:
    constexpr EnumMap() noexcept
        requires(std::is_default_constructible_v<Value>)
    = default;

    constexpr explicit EnumMap(Value default_value) noexcept { map_.fill(default_value); }

    // Asserts that the key is a valid enumeration
    template <typename Self>
    [[nodiscard]] constexpr auto operator[](this Self&& self, E key) noexcept -> decltype(auto) {
        const auto index = magic_enum::enum_index(key);
        assert(index && "Key must be a valid enumeration");
        return self.map_[*index];
    }

    // Returns the value at the key or nullopt if contextually convertible
    //
    // Contextually convertible Values are pointers and optional types
    [[nodiscard]] constexpr auto get_opt(E key) const noexcept {
        if constexpr (is_optional_v<Value>) {
            return operator[](key);
        } else if constexpr (std::is_pointer_v<Value>) {
            const auto value = operator[](key);
            return value ? Optional<Value>{value} : std::nullopt;
        } else {
            return Optional<Value>{operator[](key)};
        }
    }

  private:
    Map map_{};
};

// Returns an inclusive range of enum values. Requires `lower < higher`
template <auto Lower, auto Upper>
    requires(BoundedEnum<decltype(Lower)> && std::same_as<decltype(Lower), decltype(Upper)>)
consteval auto enum_range() noexcept {
    using E                 = decltype(Lower);
    constexpr auto opt_low  = magic_enum::enum_index<E>(Lower);
    constexpr auto opt_high = magic_enum::enum_index<E>(Upper);
    static_assert(opt_low && opt_high, "Bounds must be valid enumerations");

    constexpr usize low_idx  = *opt_low;
    constexpr usize high_idx = *opt_high;
    static_assert(high_idx >= low_idx, "Range must be strictly increasing");

    // The range is inclusive to circumvent weird indexing
    constexpr usize count = high_idx - low_idx + 1;
    return []<usize... Is>(std::index_sequence<Is...>, usize offset) {
        return std::array<E, sizeof...(Is)>{enum_value<E>(offset + Is)...};
    }(std::make_index_sequence<count>{}, low_idx);
}

// Returns an array of all possible enum values
template <BoundedEnum E> consteval auto enum_range() noexcept {
    return enum_range<enum_min_value<E>(), enum_max_value<E>()>();
}

} // namespace porpoise

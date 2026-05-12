#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace porpoise {

using u8    = std::uint8_t;
using i8    = std::int8_t;
using u16   = std::uint16_t;
using i16   = std::int16_t;
using u32   = std::uint32_t;
using i32   = std::int32_t;
using u64   = std::uint64_t;
using i64   = std::int64_t;
using usize = std::size_t;
using isize = std::make_signed_t<usize>;
using uptr  = std::uintptr_t;
using iptr  = std::intptr_t;
using idiff = std::ptrdiff_t;

using f32 = float;
using f64 = double;

using byte = std::string_view::value_type;

template <typename T>
concept Integral = std::is_integral_v<T> && !std::same_as<T, bool>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept ScopedEnum = std::is_scoped_enum_v<T>;

template <typename T>
concept Reference = std::is_reference_v<T>;

template <class T>
concept PairLike = requires {
    typename std::tuple_size<std::decay_t<T>>::type;
    requires std::tuple_size_v<std::decay_t<T>> == 2;
};

} // namespace porpoise

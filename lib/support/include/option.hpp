#pragma once

#include <cassert>
#include <concepts>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

#include "types.hpp"

namespace porpoise::opt {

using None          = std::nullopt_t;
constexpr None none = std::nullopt;

namespace detail {

template <typename T> class Ref {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr Ref() noexcept : ptr_{nullptr} {}
    constexpr Ref(None) noexcept : ptr_{nullptr} {}
    constexpr Ref(T& ref) noexcept : ptr_{&ref} {}
    constexpr Ref(T* ref) noexcept : ptr_{ref} {}
    Ref(T&&) = delete;

    template <typename U>
        requires(std::convertible_to<U*, T*>)
    constexpr Ref(const Ref<U>& other) noexcept : ptr_{other.operator->()} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto     has_value() const noexcept -> bool { return ptr_ != nullptr; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto emplace(T& t) noexcept -> void { ptr_ = &t; }
    constexpr auto emplace(T* t) noexcept -> void { ptr_ = t; }
    constexpr auto reset() noexcept -> void { ptr_ = nullptr; }

    // Resets the optional and returns the stored reference
    [[nodiscard]] constexpr auto take() noexcept -> T* {
        assert(ptr_ && "Attempt to access empty optional reference");
        auto* ptr = ptr_;
        reset();
        return ptr;
    }

    [[nodiscard]] constexpr auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    [[nodiscard]] constexpr auto get() const noexcept -> T& {
        assert(has_value() && "Attempt to access empty optional reference");
        return *ptr_;
    }

    [[nodiscard]] constexpr auto operator->() const noexcept -> T* {
        assert(ptr_ && "Attempt to access empty optional reference");
        return ptr_;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> T& { return get(); }

    // Applies F to to underlying reference if present
    template <typename Self, class F>
    [[nodiscard]] constexpr auto transform(this Self&& self, F&& f) {
        using ResCV = std::invoke_result_t<F, T&>;
        using Res   = std::remove_cv_t<ResCV>;

        // This is straight from clang's stdc++ C++23 optional implementation
        static_assert(!std::is_array_v<Res>, "Result of f(value()) should not be an Array");
        static_assert(!std::is_same_v<Res, std::in_place_t>,
                      "Result of f(value()) should not be std::in_place_t");
        static_assert(!std::is_same_v<Res, None>, "Result of f(value()) should not be opt::none");
        static_assert(std::is_object_v<Res>, "Result of f(value()) should be an object type");

        // Also from clang, but generalized to support reference transform chains
        using Ret = std::conditional_t<std::is_reference_v<ResCV>,
                                       Ref<std::remove_reference_t<ResCV>>,
                                       std::optional<ResCV>>;
        if (self.has_value()) { return Ret{std::forward<F>(f)(self.value())}; }
        return Ret{};
    }

  private:
    T* ptr_;
};

// An efficient optional representation of boolean values
class Boolean {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr Boolean() noexcept : value_{NO_VALUE} {}
    constexpr Boolean(bool value) noexcept : value_{static_cast<u8>(value)} {}
    constexpr Boolean(None) noexcept : value_{NO_VALUE} {}
    constexpr Boolean(const std::optional<bool>& ob) noexcept
        : value_{ob.transform([](bool b) { return static_cast<u8>(b); }).value_or(NO_VALUE)} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return value_ != NO_VALUE; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto emplace(bool value) noexcept -> void { value_ = value; }
    constexpr auto reset() noexcept -> void { value_ = NO_VALUE; }

    // Resets the optional and returns the stored bool
    [[nodiscard]] constexpr auto take() noexcept -> bool {
        const auto value = value_;
        reset();
        return value;
    }

    [[nodiscard]] constexpr auto value() const -> bool {
        if (!has_value()) { throw std::bad_optional_access(); }
        return static_cast<bool>(value_);
    }

    [[nodiscard]] constexpr auto get() const noexcept -> bool {
        assert(has_value() && "Attempt to access empty optional boolean");
        return static_cast<bool>(value_);
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> bool { return get(); }

    template <class Or> [[nodiscard]] constexpr auto value_or(Or&& or_value) -> bool {
        // This is straight from clang's stdc++ C++23 optional implementation
        static_assert(std::is_copy_constructible_v<Or>,
                      "Boolean::value_or: T must be copy constructible");
        static_assert(std::is_convertible_v<Or, bool>,
                      "Boolean::value_or: Or must be convertible to T");
        return has_value() ? this->get() : static_cast<bool>(std::forward<Or>(or_value));
    }

    [[nodiscard]] constexpr operator std::optional<bool>() const noexcept {
        return has_value() ? std::optional<bool>{value_} : opt::none;
    }

  private:
    static constexpr u8 NO_VALUE = 3;

  private:
    u8 value_;
};

} // namespace detail

// A safe, reference-allowable optional type dispatcher
template <typename T>
using Option = std::conditional_t<
    std::is_reference_v<T>,
    detail::Ref<std::remove_reference_t<T>>,
    std::conditional_t<std::is_same_v<T, bool>, detail::Boolean, std::optional<T>>>;

template <typename T> struct is_option : std::false_type {};
template <typename T> struct is_option<std::optional<T>> : std::true_type {};
template <typename T> struct is_option<detail::Ref<T>> : std::true_type {};
template <typename T> constexpr bool is_option_v = is_option<T>::value;

// Compares two values, forwarding safety concerns to the comparator.
template <typename T, typename Comparator>
constexpr auto safe_eq(const Option<T>& a, const Option<T>& b, Comparator cmp) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(*a, *b);
}

// Compares two values, delegating equality to the default equality operator.
template <typename T>
constexpr auto safe_eq(const Option<T>& a, const Option<T>& b) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return *a == *b;
}

#define MAKE_OPTIONAL_UNPACKER(name, ConstReturnType, member, deref)                           \
    [[nodiscard]] auto get_##name() const noexcept -> ConstReturnType { return deref member; } \
    [[nodiscard]] auto has_##name() const noexcept -> bool { return member.has_value(); }

// A minimal, zero-cost optional usize wrapper
class Index {
  public:
    constexpr Index() noexcept = default;

    // cppcheck-suppress-begin noExplicitConstructor
    constexpr Index(usize idx) noexcept : idx_{idx} {}
    constexpr Index(std::nullopt_t) noexcept {}

    // Any negative value is treated as a sentinel
    template <Integral Int> constexpr Index(Int i) noexcept {
        if (i >= 0) { idx_ = static_cast<usize>(i); }
    }

    constexpr Index(const std::optional<usize>& oi) noexcept : idx_{oi.value_or(NO_VALUE)} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto     has_value() const noexcept -> bool { return idx_ != NO_VALUE; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto emplace(usize idx) noexcept -> void { idx_ = idx; }

    constexpr auto               reset() noexcept -> void { idx_ = NO_VALUE; }
    [[nodiscard]] constexpr auto take() noexcept -> usize {
        const usize idx = idx_;
        reset();
        return idx;
    }

    [[nodiscard]] constexpr auto value() const -> usize {
        if (!*this) { throw std::bad_optional_access(); }
        return idx_;
    }

    [[nodiscard]] constexpr auto get() const noexcept -> usize {
        assert(has_value() && "Attempt to access empty optional enum");
        return idx_;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> usize { return get(); }

    [[nodiscard]] constexpr operator std::optional<usize>() const noexcept {
        return has_value() ? std::optional<usize>{idx_} : opt::none;
    }

  private:
    static constexpr usize NO_VALUE = std::numeric_limits<usize>::max();

  private:
    usize idx_{NO_VALUE};
};

// Defines a sentinel value for the enum that is the same as its underlying type
template <typename E> struct SentinelEnum;

template <ScopedEnum E> struct SentinelEnum<E> {
    static constexpr auto SENTINEL = std::numeric_limits<std::underlying_type_t<E>>::max();
};

template <typename E>
concept OptionableEnum = ScopedEnum<E> && requires { static_cast<E>(SentinelEnum<E>::SENTINEL); };

// An efficient optional enum representation for enums with a sentinel value
template <OptionableEnum E> class Enum {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr Enum() noexcept : value_{NO_VALUE} {}
    constexpr Enum(E value) noexcept : value_{value} {}
    constexpr Enum(None) noexcept : value_{NO_VALUE} {}
    constexpr Enum(const std::optional<E>& oe) noexcept : value_{oe.value_or(NO_VALUE)} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return value_ != NO_VALUE; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto emplace(E value) noexcept -> void { value_ = value; }
    constexpr auto reset() noexcept -> void { value_ = NO_VALUE; }

    // Resets the optional and returns the stored value
    [[nodiscard]] constexpr auto take() noexcept -> E {
        auto value = value_;
        reset();
        return value;
    }

    [[nodiscard]] constexpr auto value() const -> E {
        if (!has_value()) { throw std::bad_optional_access(); }
        return value_;
    }

    [[nodiscard]] constexpr auto get() const noexcept -> E {
        assert(has_value() && "Attempt to access empty optional enum");
        return value_;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> E { return get(); }
    [[nodiscard]] friend auto    operator==(const Enum& lhs, const Enum& rhs) noexcept
        -> bool = default;

    template <typename Self, class F>
    [[nodiscard]] constexpr auto transform(this Self&& self, F&& f) {
        using Res = std::remove_cv_t<std::invoke_result_t<F, E>>;

        // This is straight from clang's stdc++ C++23 optional implementation
        static_assert(!std::is_array_v<Res>, "Result of f(value()) should not be an Array");
        static_assert(!std::is_same_v<Res, std::in_place_t>,
                      "Result of f(value()) should not be std::in_place_t");
        static_assert(!std::is_same_v<Res, None>, "Result of f(value()) should not be opt::none");
        static_assert(std::is_object_v<Res>, "Result of f(value()) should be an object type");

        // Also from clang, but generalized to support reference transform chains
        if (self.has_value()) { return Option<Res>{std::forward<F>(f)(self.value())}; }
        return Option<Res>{};
    }

    [[nodiscard]] constexpr operator std::optional<E>() const noexcept {
        return has_value() ? std::optional<E>{value_} : opt::none;
    }

  private:
    static constexpr E NO_VALUE = static_cast<E>(SentinelEnum<E>::SENTINEL);

  private:
    E value_;
};

} // namespace porpoise::opt

#pragma once

#include <concepts>
#include <limits>
#include <optional>
#include <type_traits>

#include "assert.hpp"
#include "types.hpp"

namespace porpoise {

namespace traits {

template <typename T> struct Nullable;

// When true, allows the type to be used in a compact optional representation
template <typename T>
concept Compactable = !Reference<T> && requires(const T& t) {
    { Nullable<T>::invalid() } -> std::same_as<T>;
    { Nullable<T>::is_valid(t) } -> std::same_as<bool>;
};

template <ScopedEnum E> struct Nullable<E> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> E {
        return static_cast<E>(std::numeric_limits<std::underlying_type_t<E>>::max());
    }

    [[nodiscard]] static constexpr auto is_valid(const E& e) noexcept -> bool {
        return e != invalid();
    }
};

} // namespace traits

namespace opt {

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
        ASSERT(ptr_, "Attempt to access empty optional reference");
        auto* ptr = ptr_;
        reset();
        return ptr;
    }

    [[nodiscard]] constexpr auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    [[nodiscard]] constexpr auto get() const noexcept -> T& {
        ASSERT(has_value(), "Attempt to access empty optional reference");
        return *ptr_;
    }

    [[nodiscard]] constexpr auto operator->() const noexcept -> T* {
        ASSERT(ptr_, "Attempt to access empty optional reference");
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

// Returns the Ref version of the optional type if required
template <typename T>
using TryDispatchRef =
    std::conditional_t<Reference<T>, Ref<std::remove_reference_t<T>>, std::optional<T>>;

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
        ASSERT(has_value(), "Attempt to access empty optional boolean");
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

// An efficient optional enum representation for enums with a sentinel value
template <traits::Compactable T> class CompactOpt {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr CompactOpt() noexcept : value_{NO_VALUE} {}
    constexpr CompactOpt(const T& value) noexcept : value_{value} {}
    constexpr CompactOpt(None) noexcept : value_{NO_VALUE} {}
    constexpr CompactOpt(const std::optional<T>& opt) noexcept : value_{opt.value_or(NO_VALUE)} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] constexpr auto has_value() const noexcept -> bool {
        return traits::Nullable<T>::is_valid(value_);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    constexpr auto emplace(const T& value) noexcept -> void { value_ = value; }
    constexpr auto reset() noexcept -> void { value_ = NO_VALUE; }

    // Resets the optional and returns the stored value
    [[nodiscard]] constexpr auto take() noexcept -> T {
        auto value = value_;
        reset();
        return value;
    }

    [[nodiscard]] constexpr auto value() const -> T {
        if (!has_value()) { throw std::bad_optional_access(); }
        return value_;
    }

    [[nodiscard]] constexpr auto get() const noexcept -> T {
        ASSERT(has_value(), "Attempt to access empty compact optional");
        return value_;
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> T { return get(); }
    [[nodiscard]] friend auto    operator==(const CompactOpt& lhs, const CompactOpt& rhs) noexcept
        -> bool = default;

    template <typename Self, class F>
    [[nodiscard]] constexpr auto transform(this Self&& self, F&& f) {
        using Res = std::remove_cv_t<std::invoke_result_t<F, T>>;

        // This is straight from clang's stdc++ C++23 optional implementation
        static_assert(!std::is_array_v<Res>, "Result of f(value()) should not be an Array");
        static_assert(!std::is_same_v<Res, std::in_place_t>,
                      "Result of f(value()) should not be std::in_place_t");
        static_assert(!std::is_same_v<Res, None>, "Result of f(value()) should not be opt::none");
        static_assert(std::is_object_v<Res>, "Result of f(value()) should be an object type");

        // Also from clang, but generalized to support reference transform chains
        using Ret = TryDispatchRef<Res>;
        return self.has_value() ? Ret{std::forward<F>(f)(self.value())} : Ret{};
    }

    [[nodiscard]] constexpr operator std::optional<T>() const noexcept {
        return has_value() ? std::optional<T>{value_} : opt::none;
    }

  private:
    static constexpr T NO_VALUE = traits::Nullable<T>::invalid();

  private:
    T value_;
};

template <typename T> struct OptionImpl {
    using type = std::optional<T>;
};

template <Reference T> struct OptionImpl<T> {
    using type = Ref<std::remove_reference_t<T>>;
};

template <> struct OptionImpl<bool> {
    using type = Boolean;
};

template <traits::Compactable T> struct OptionImpl<T> {
    using type = CompactOpt<T>;
};

} // namespace detail

// A safe, reference-allowable optional type dispatcher
template <typename T> using Option = typename detail::OptionImpl<T>::type;

template <typename T> struct is_option : std::false_type {};
template <typename T> struct is_option<std::optional<T>> : std::true_type {};
template <typename T> struct is_option<detail::Ref<T>> : std::true_type {};
template <> struct is_option<detail::Boolean> : std::true_type {};
template <typename T> struct is_option<detail::CompactOpt<T>> : std::true_type {};
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
        ASSERT(has_value(), "Attempt to access empty optional enum");
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

} // namespace opt

} // namespace porpoise

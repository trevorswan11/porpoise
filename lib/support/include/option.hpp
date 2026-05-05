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
    Ref() noexcept : ptr_{nullptr} {}
    Ref(None) noexcept : ptr_{nullptr} {}
    Ref(T& ref) noexcept : ptr_{&ref} {}
    Ref(T* ref) noexcept : ptr_{ref} {}
    Ref(T&&) = delete;

    template <typename U>
        requires(std::convertible_to<U*, T*>)
    Ref(const Ref<U>& other) noexcept : ptr_{other.operator->()} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] auto     has_value() const noexcept -> bool { return ptr_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    auto emplace(T& t) noexcept -> void { ptr_ = &t; }
    auto emplace(T* t) noexcept -> void { ptr_ = t; }
    auto reset() noexcept -> void { ptr_ = nullptr; }

    // Resets the optional and returns the stored reference
    auto take() noexcept -> T* {
        assert(ptr_ && "Attempt to access empty optional reference");
        auto* ptr = ptr_;
        reset();
        return ptr;
    }

    auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    auto operator->() const noexcept -> T* {
        assert(ptr_ && "Attempt to access empty optional reference");
        return ptr_;
    }

    auto operator*() const noexcept -> T& {
        assert(ptr_ && "Attempt to access empty optional reference");
        return *ptr_;
    }

    // Applies F to to underlying reference if present
    template <class F> constexpr auto transform(F&& f) & {
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
        if (*this) { return Ret{std::forward<F>(f)(value())}; }
        return Ret{};
    }

  private:
    T* ptr_;
};

// An efficient optional representation of boolean values
class Boolean {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    Boolean() noexcept : value_{NO_VALUE} {}
    Boolean(bool value) noexcept : value_{static_cast<u8>(value)} {}
    Boolean(None) noexcept : value_{NO_VALUE} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] auto     has_value() const noexcept -> bool { return value_ != NO_VALUE; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    auto emplace(bool value) noexcept -> void { value_ = value; }
    auto reset() noexcept -> void { value_ = NO_VALUE; }

    // Resets the optional and returns the stored bool
    auto take() noexcept -> bool {
        auto value = value_;
        reset();
        return value;
    }

    auto value() const -> bool {
        if (!has_value()) { throw std::bad_optional_access(); }
        return static_cast<bool>(value_);
    }

    auto get() const noexcept -> bool {
        assert(has_value() && "Attempt to access empty optional boolean");
        return static_cast<bool>(value_);
    }

    template <class Or> constexpr auto value_or(Or&& or_value) -> bool {
        // This is straight from clang's stdc++ C++23 optional implementation
        static_assert(std::is_copy_constructible_v<Or>,
                      "Boolean::value_or: T must be copy constructible");
        static_assert(std::is_convertible_v<Or, bool>,
                      "Boolean::value_or: Or must be convertible to T");
        return has_value() ? this->get() : static_cast<bool>(std::forward<Or>(or_value));
    }

    auto operator*() const noexcept -> bool {
        assert(has_value() && "Attempt to access empty optional boolean");
        return value_;
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
auto safe_eq(const Option<T>& a, const Option<T>& b, Comparator cmp) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(*a, *b);
}

// Compares two values, delegating equality to the default equality operator.
template <typename T> auto safe_eq(const Option<T>& a, const Option<T>& b) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return *a == *b;
}

#define MAKE_OPTIONAL_UNPACKER(name, ReturnType, member, deref)                                  \
    [[nodiscard]] auto get_##name() const noexcept -> const ReturnType& { return deref member; } \
    [[nodiscard]] auto has_##name() const noexcept -> bool { return member.has_value(); }

// A minimal usize wrapper that provides zero-cost optional behavior
class Index {
  public:
    Index() noexcept = default;

    // cppcheck-suppress-begin noExplicitConstructor
    Index(usize idx) noexcept : idx_{idx} {}
    Index(std::nullopt_t) noexcept {}

    // Any negative value is treated as a sentinel
    template <Integral Int> Index(Int i) noexcept {
        if (i >= 0) { idx_ = static_cast<usize>(i); }
    }
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] auto     has_value() const noexcept -> bool { return idx_ != NO_VALUE; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    auto emplace(usize idx) noexcept -> void { idx_ = idx; }

    auto reset() noexcept -> void { idx_ = NO_VALUE; }
    auto take() noexcept -> usize {
        usize idx = idx_;
        reset();
        return idx;
    }

    auto value() const -> usize {
        if (!*this) { throw std::bad_optional_access(); }
        return idx_;
    }

    auto operator*() const noexcept -> usize {
        assert(has_value() && "Attempt to access empty optional boolean");
        return idx_;
    }

  private:
    static constexpr usize NO_VALUE = std::numeric_limits<usize>::max();

  private:
    usize idx_{NO_VALUE};
};

} // namespace porpoise::opt

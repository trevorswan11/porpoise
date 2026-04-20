#pragma once

#include <cassert>
#include <concepts>
#include <optional>
#include <type_traits>

namespace porpoise::opt {

using None          = std::nullopt_t;
constexpr None none = std::nullopt;

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
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    auto emplace(T& t) noexcept -> void { ptr_ = &t; }
    auto emplace(T* t) noexcept -> void { ptr_ = t; }
    auto reset() noexcept -> void { ptr_ = nullptr; }

    // Resets the optional and returns the stored reference
    auto take() noexcept -> T* {
        auto* ptr = ptr_;
        reset();
        return ptr;
    }

    auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    auto operator->() const noexcept -> T* { return ptr_; }
    auto operator*() const noexcept -> T& { return *ptr_; }

  private:
    T* ptr_;
};

// A safe, reference-allowable optional type dispatcher
template <typename T>
using Option =
    std::conditional_t<std::is_reference_v<T>, Ref<std::remove_reference_t<T>>, std::optional<T>>;

template <typename T> struct is_option : std::false_type {};
template <typename T> struct is_option<std::optional<T>> : std::true_type {};
template <typename T> struct is_option<Ref<T>> : std::true_type {};
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

// A non-null pointer for use when a reference is inappropriate.
template <typename T>
    requires(!std::is_reference_v<T>)
class NonNull {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    NonNull(T* ptr) noexcept : ptr_{ptr} {
        assert(ptr_ && "Attempt to create NonNull from nullptr");
    }
    NonNull(Ref<T> opt) : ptr_{&opt.value()} {}
    NonNull(None) = delete;
    NonNull(T&&)  = delete;

    template <typename U>
        requires(std::convertible_to<U*, T*>)
    NonNull(const NonNull<U>& other) noexcept : ptr_{other.get()} {}
    // cppcheck-suppress-end noExplicitConstructor

    auto operator->() const noexcept -> T* { return ptr_; }
    auto operator*() const noexcept -> T& { return *ptr_; }
    auto get() const noexcept -> T* { return ptr_; }

    explicit operator T() const noexcept { return *ptr_; }
    bool     operator==(const NonNull<T>&) const noexcept = default;

  private:
    T* ptr_;
};

} // namespace porpoise::opt

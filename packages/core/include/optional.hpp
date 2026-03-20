#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <type_traits>

namespace porpoise {

template <typename T> class OptionalRef {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    OptionalRef() noexcept : ptr_{nullptr} {}
    OptionalRef(std::nullopt_t) noexcept : ptr_{nullptr} {}
    OptionalRef(T& ref) noexcept : ptr_{&ref} {}
    OptionalRef(T&&) = delete;

    template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = false>
    OptionalRef(const OptionalRef<U>& other) noexcept : ptr_{other.operator->()} {}
    // cppcheck-suppress-end noExplicitConstructor

    [[nodiscard]] auto     has_value() const noexcept -> bool { return ptr_ != nullptr; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    auto value() const -> T& {
        if (!ptr_) { throw std::bad_optional_access(); }
        return *ptr_;
    }

    auto operator->() const noexcept -> T* { return ptr_; }
    auto operator*() const noexcept -> T& { return *ptr_; }

  private:
    T* ptr_;
};

template <typename T>
using Optional = std::conditional_t<std::is_reference_v<T>,
                                    OptionalRef<std::remove_reference_t<T>>,
                                    std::optional<T>>;

#define MAKE_OPTIONAL_UNPACKER(name, ReturnType, member, deref)                                  \
    [[nodiscard]] auto get_##name() const noexcept -> const ReturnType& { return deref member; } \
    [[nodiscard]] auto has_##name() const noexcept -> bool { return member.has_value(); }

// A non-null pointer for use when a reference is inappropriate.
template <typename T>
    requires(!std::is_reference_v<T>)
class NonNull {
  public:
    explicit NonNull(T* ptr) noexcept : ptr_{ptr} {
        assert(ptr_ && "Attempt to create NonNull from nullptr");
    }
    explicit NonNull(OptionalRef<T> opt) : ptr_{&opt.value()} {}
    NonNull(std::nullopt_t) = delete;
    NonNull(T&&)            = delete;

    template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = false>
    explicit NonNull(const NonNull<U>& other) noexcept : ptr_{other.get()} {}

    auto operator->() const noexcept -> T* { return ptr_; }
    auto operator*() const noexcept -> T& { return *ptr_; }
    auto get() const noexcept -> T* { return ptr_; }

  private:
    T* ptr_;
};

namespace optional {

template <typename T>
using Comparator = bool (*)(const std::remove_reference_t<T>&, const std::remove_reference_t<T>&);

// Compares two values, forwarding safety concerns to the comparator.
template <typename T>
auto safe_eq(const Optional<T>& a, const Optional<T>& b, Comparator<T> cmp) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(*a, *b);
}

// Compares two values, delegating equality to the default equality operator.
template <typename T> auto safe_eq(const Optional<T>& a, const Optional<T>& b) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return *a == *b;
}

// Compares two unique pointers by assuming that both are valid.
template <typename T>
auto unsafe_eq(const Optional<std::unique_ptr<T>>& a,
               const Optional<std::unique_ptr<T>>& b,
               Comparator<T>                       cmp) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(**a, **b);
}

// Compares two unique pointers by assuming that both are valid, using the default equality
// operator.
template <typename T>
auto unsafe_eq(const Optional<std::unique_ptr<T>>& a,
               const Optional<std::unique_ptr<T>>& b) noexcept -> bool {
    return unsafe_eq<T>(a, b, [](const T& ae, const T& be) { return ae == be; });
}

} // namespace optional

} // namespace porpoise

#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "optional.hpp"

namespace porpoise {

namespace mem {

// cppcheck-suppress-begin noExplicitConstructor

// An alias for a unique pointer which should be seldom used
template <typename T, typename D = std::default_delete<T>>
using NullableBox = std::unique_ptr<T, D>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_nullable_box(Args&&... args) -> NullableBox<T> {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Makes a new nullable box from an existing pointer
template <typename T, typename P>
[[nodiscard]] constexpr auto nullable_box_from(P* ptr) -> NullableBox<T> {
    return std::unique_ptr<T>{static_cast<T*>(ptr)};
}

// Makes a new nullable box from an existing box, changing the type as requested
template <typename T, typename P>
[[nodiscard]] constexpr auto nullable_box_into(NullableBox<P>&& ptr) -> NullableBox<T> {
    return std::unique_ptr<T>{static_cast<T*>(ptr.release())};
}

template <typename T> struct is_nullable_box : std::false_type {};
template <typename T, typename D> struct is_nullable_box<NullableBox<T, D>> : std::true_type {};
template <typename T> constexpr bool is_nullable_box_v = is_nullable_box<T>::value;

struct NullBoxException : public std::logic_error {
    NullBoxException()
        : std::logic_error{"Violated Invariant: Attempted to initialize with null"} {}
    explicit NullBoxException(std::string message) : std::logic_error{std::move(message)} {}
};

// A light unique pointer wrapper that ensures pointer validity at initialization.
//
// All methods besides constructors and factories assert this invariant.
template <typename T, typename D = std::default_delete<T>> class Box {
  public:
    explicit Box(NullableBox<T, D>&& ptr) : ptr_{std::move(ptr)} {
        if (!ptr_) { throw NullBoxException{}; }
    }

    template <typename P> explicit Box(P* ptr) : ptr_{NullableBox<T>{static_cast<T*>(ptr)}} {
        if (!ptr_) { throw NullBoxException{}; }
    }

    template <typename U, typename E>
        requires(std::is_convertible_v<U*, T*>)
    Box(Box<U, E>&& other) : ptr_{std::move(other.ptr_)} {}

    ~Box()                             = default;
    Box(const Box&)                    = delete;
    auto operator=(const Box&) -> Box& = delete;
    Box(Box&&)                         = default;
    auto operator=(Box&&) -> Box&      = default;

    [[nodiscard]] auto operator*() const noexcept -> T& {
        assert(ptr_ && "Attempted to dereference a moved-from Box");
        return *ptr_;
    }

    [[nodiscard]] auto operator->() const noexcept -> T* {
        assert(ptr_ && "Attempted to access a moved-from Box");
        return get();
    }

    [[nodiscard]] auto release() noexcept -> T* {
        assert(ptr_ && "Attempted to release a moved-from Box");
        return ptr_.release();
    }

    [[nodiscard]] auto get() const noexcept -> T* {
        assert(ptr_ && "Attempted to access a moved-from Box");
        return ptr_.get();
    }

    template <typename... Args> [[nodiscard]] static auto make(Args&&... args) -> Box<T> {
        return Box{std::make_unique<T>(std::forward<Args>(args)...)};
    }

    operator bool() const noexcept { return ptr_.operator bool(); }

  private:
    NullableBox<T, D> ptr_;

    template <typename U, typename E> friend class Box;
};

template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_box(Args&&... args) -> Box<T> {
    return Box<T>::make(std::forward<Args>(args)...);
}

// Makes a new box from an existing pointer
template <typename T, typename P> [[nodiscard]] constexpr auto box_from(P* ptr) -> Box<T> {
    return Box<T>{ptr};
}

// Makes a new box from an existing box, changing the type as requested
template <typename T, typename P> [[nodiscard]] constexpr auto box_into(Box<P>&& ptr) -> Box<T> {
    return Box<T>{ptr.release()};
}

template <typename T> struct is_box : std::false_type {};
template <typename T, typename D> struct is_box<Box<T, D>> : std::true_type {};
template <typename T> constexpr bool is_box_v = is_box<T>::value;

// If you find yourself using this, think really hard about the decisions that led you here...
template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args>
[[nodiscard]] constexpr auto make_rc(Args&&... args) -> Rc<T> {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// cppcheck-suppress-end noExplicitConstructor

} // namespace mem

namespace optional {

// Compares two boxes based on the provided comparator
template <typename T>
auto unsafe_eq(const Optional<mem::Box<T>>& a,
               const Optional<mem::Box<T>>& b,
               Comparator<T>                cmp) noexcept -> bool {
    if (a.has_value() != b.has_value()) { return false; }
    if (!a.has_value()) { return true; }
    return cmp(**a, **b);
}

// Compares two boxes by using the underlying type's default equality operator
template <typename T>
auto unsafe_eq(const Optional<mem::Box<T>>& a, const Optional<mem::Box<T>>& b) noexcept -> bool {
    return unsafe_eq<T>(a, b, [](const T& ae, const T& be) { return ae == be; });
}

} // namespace optional

} // namespace porpoise

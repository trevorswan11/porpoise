#pragma once

#include <type_traits>

#include "types.hpp"

namespace porpoise {

template <typename T>
concept Countable = std::is_integral_v<T> && !std::is_same_v<T, bool>;

// A simple counter that provides RAII-based up/down counting
template <Countable Underlying> class Counter {
  public:
    class Guard {
      public:
        explicit Guard(Counter& c) : c_{c} { c_.increment(); }
        ~Guard() { c_.decrement(); }

      private:
        Counter& c_;
    };

  public:
    auto increment() noexcept -> void { count_ += ONE; }
    auto decrement() noexcept -> void { count_ -= ONE; }

    operator bool() noexcept { return count_ != ZERO; }
    operator Underlying() noexcept { return static_cast<Underlying>(count_); }

    auto operator<=>(const Counter&) const noexcept        = default;
    auto operator==(const Counter&) const noexcept -> bool = default;

    template <typename T>
        requires(std::is_convertible_v<T, Underlying>)
    auto operator==(const T& other) const noexcept -> bool {
        return count_ == static_cast<Underlying>(other);
    }

    template <typename T>
        requires(std::is_convertible_v<T, Underlying>)
    auto operator<=>(const T& other) const noexcept {
        return count_ <=> static_cast<Underlying>(other);
    }

    // Creates an RAII incrementor/decrementor object
    [[nodiscard]] auto guard() noexcept -> Guard { return Guard{*this}; }

  private:
    static constexpr auto ZERO = static_cast<Underlying>(0);
    static constexpr auto ONE  = static_cast<Underlying>(1);

  private:
    Underlying count_{ZERO};
};

using DefaultCounter = Counter<usize>;

} // namespace porpoise

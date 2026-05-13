#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

#include "assert.hh"
#include "types.hh"

namespace porpoise {

// A fixed-size zero-allocation container with a vector-like interface
template <typename Item, usize Capacity> class StaticVector {
  public:
    using value_type      = Item;
    using size_type       = usize;
    using reference       = Item&;
    using const_reference = const Item&;
    using iterator        = Item*;
    using const_iterator  = const Item*;

  public:
    StaticVector() = default;
    ~StaticVector() { clear(); }
    ~StaticVector()
        requires(TriviallyDestructible<Item>)
    = default;

    // Constructs the vector in place by emplacing each item into the buffer
    template <typename... Is> constexpr explicit StaticVector(Is&&... items) {
        static_assert(sizeof...(Is) <= Capacity, "Cannot initialize with more items than capacity");
        (..., emplace_back(std::forward<Is>(items)));
    }

    constexpr StaticVector(const StaticVector&)
        requires(TriviallyCopyable<Item>)
    = default;

    constexpr StaticVector(const StaticVector& other) {
        if constexpr (TriviallyCopyable<Item>) {
            size_ = other.size_;
            std::memcpy(items_, other.items_, size_ * sizeof(Item));
        } else {
            for (const auto& item : other) { emplace_back(item); }
        }
    }

    constexpr StaticVector& operator=(const StaticVector&)
        requires(TriviallyCopyable<Item>)
    = default;

    auto operator=(const StaticVector& other) -> StaticVector& {
        if (this != &other) {
            StaticVector temp{other};
            using std::swap;
            swap(*this, temp);
        }
        return *this;
    }

    constexpr StaticVector(StaticVector&& other) noexcept {
        if constexpr (TriviallyCopyable<Item>) {
            size_ = other.size_;
            std::memcpy(items_, other.items_, size_ * sizeof(Item));
        } else {
            for (auto& item : other) { emplace_back(std::move(item)); }
        }
        other.clear();
    }

    constexpr auto operator=(StaticVector&& other) -> StaticVector& {
        if (this != &other) {
            clear();
            if constexpr (TriviallyCopyable<Item>) {
                size_ = other.size_;
                std::memcpy(items_, other.items_, size_ * sizeof(Item));
            } else {
                for (auto& item : other) { emplace_back(std::move(item)); }
            }
            other.clear();
        }
        return *this;
    }

    template <typename... Args> constexpr auto emplace_back(Args&&... args) -> void {
        if (size_ >= Capacity) { throw std::out_of_range{"StaticVector size out of range"}; }
        new (data() + size_) Item{std::forward<Args>(args)...};
        size_++;
    }

    constexpr auto push_back(const Item& item) -> void { emplace_back(item); }
    constexpr auto push_back(Item&& item) -> void { emplace_back(std::move(item)); }

    [[nodiscard]] constexpr explicit operator std::span<Item>() noexcept { return {data(), size_}; }

    [[nodiscard]] constexpr explicit operator std::span<const Item>() const noexcept {
        return {data(), size_};
    }

    template <typename Self>
    [[nodiscard]] constexpr auto operator[](this Self&& self, usize idx) noexcept
        -> decltype(auto) {
        ASSERT(idx < self.size_, "StaticVector index out of bounds");
        return self.data()[idx];
    }

    [[nodiscard]] constexpr auto begin() noexcept -> iterator { return data(); }
    [[nodiscard]] constexpr auto end() noexcept -> iterator { return data() + size_; }
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return data(); }
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator { return data() + size_; }

    // Ambiguous but required for iterator, only checks for emptiness
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return size_ == 0; }
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return size_; }

    [[nodiscard]] constexpr auto data() noexcept -> Item* {
        return reinterpret_cast<Item*>(items_);
    }

    [[nodiscard]] constexpr auto data() const noexcept -> const Item* {
        return reinterpret_cast<const Item*>(items_);
    }

    constexpr auto clear() noexcept -> void {
        // The lion is now concerned with freeing non-trivial resources
        if constexpr (!TriviallyDestructible<Item>) {
            for (usize i = 0; i < size_; ++i) { data()[i].~Item(); }
        }
        size_ = 0;
    }

  private:
    // https://en.cppreference.com/cpp/algorithm/swap
    constexpr auto swap(StaticVector& other) noexcept -> void {
        if constexpr (TriviallyCopyable<Item>) {
            const usize max_size = std::max(size_, other.size_);
            std::swap_ranges(items_, items_ + (max_size * sizeof(Item)), other.items_);
            std::swap(size_, other.size_);
        } else {
            auto& smaller = (size_ < other.size_) ? *this : other;
            auto& larger  = (size_ < other.size_) ? other : *this;

            std::swap_ranges(smaller.begin(), smaller.end(), larger.begin());
            const auto smaller_size = smaller.size_;
            const auto larger_size  = larger.size_;

            // Manually destroy the moved-from object after moving it
            for (usize i = smaller_size; i < larger_size; ++i) {
                smaller.emplace_back(std::move(larger[i]));
                larger.data()[i].~Item();
            }

            smaller.size_ = larger_size;
            larger.size_  = smaller_size;
        }
    }

    // ADL dispatcher for copy assignment
    constexpr friend auto swap(StaticVector& lhs, StaticVector& rhs) noexcept -> void {
        lhs.swap(rhs);
    }

  private:
    alignas(Item) std::byte items_[Capacity * sizeof(Item)];
    usize size_{0};
};

} // namespace porpoise

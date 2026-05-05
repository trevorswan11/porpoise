#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

#include "types.hpp"

namespace porpoise {

namespace detail {

template <typename Item, usize Capacity> struct StaticVectorStorage {
    alignas(Item) std::byte items_[Capacity * sizeof(Item)];
    usize size_{0};

    auto data() noexcept -> Item* { return reinterpret_cast<Item*>(items_); }

    ~StaticVectorStorage() {
        for (usize i = 0; i < size_; i++) { data()[i].~Item(); }
    }
};

// The sole reason this exists is to allow sema to hold type pointers from an arena
template <TriviallyDestructible Item, usize Capacity> struct StaticVectorStorage<Item, Capacity> {
    alignas(Item) std::byte items_[Capacity * sizeof(Item)];
    usize size_{0};

    auto data() noexcept -> Item* { return reinterpret_cast<Item*>(items_); }
};

} // namespace detail

// A fixed-size, statically-allocated & zero-allocation container
//
// Any constructable type can be stored as construction does not occur until emplacement
template <typename Item, usize Capacity>
class StaticVector : private detail::StaticVectorStorage<Item, Capacity> {
  public:
    using value_type      = Item;
    using size_type       = usize;
    using reference       = Item&;
    using const_reference = const Item&;
    using iterator        = Item*;
    using const_iterator  = const Item*;

  public:
    StaticVector() = default;

    // Constructs the vector in place by emplacing each item into the buffer
    template <typename... Is> explicit StaticVector(Is&&... items) {
        static_assert(sizeof...(Is) <= Capacity, "Cannot initialize with more items than capacity");
        (..., emplace_back(std::forward<Is>(items)));
    }

    StaticVector(const StaticVector&)
        requires(TriviallyCopyable<Item>)
    = default;

    StaticVector(const StaticVector& other) {
        if constexpr (TriviallyCopyable<Item>) {
            this->size_ = other.size_;
            std::memcpy(this->items_, other.items_, this->size_ * sizeof(Item));
        } else {
            for (const auto& item : other) { emplace_back(item); }
        }
    }

    StaticVector& operator=(const StaticVector&)
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

    StaticVector(StaticVector&& other) noexcept {
        if constexpr (TriviallyCopyable<Item>) {
            this->size_ = other.size_;
            std::memcpy(this->items_, other.items_, this->size_ * sizeof(Item));
        } else {
            for (auto& item : other) { emplace_back(std::move(item)); }
        }
        other.clear();
    }

    auto operator=(StaticVector&& other) -> StaticVector& {
        if (this != &other) {
            clear();
            if constexpr (TriviallyCopyable<Item>) {
                this->size_ = other.size_;
                std::memcpy(this->items_, other.items_, this->size_ * sizeof(Item));
            } else {
                for (auto& item : other) { emplace_back(std::move(item)); }
            }
            other.clear();
        }
        return *this;
    }

    template <typename... Args> auto emplace_back(Args&&... args) -> void {
        if (this->size_ >= Capacity) { throw std::out_of_range{"StaticVector size out of range"}; }
        new (data() + this->size_) Item{std::forward<Args>(args)...};
        this->size_++;
    }

    auto push_back(const Item& item) -> void { emplace_back(item); }
    auto push_back(Item&& item) -> void { emplace_back(std::move(item)); }

    [[nodiscard]] explicit operator std::span<Item>() noexcept { return {data(), this->size_}; }
    [[nodiscard]] explicit operator std::span<const Item>() const noexcept {
        return {data(), this->size_};
    }

    template <typename Self>
    [[nodiscard]] constexpr auto operator[](this Self&& self, usize idx) noexcept
        -> decltype(auto) {
        assert(idx < self.size_ && "StaticVector index out of bounds");
        return self.data()[idx];
    }

    // Clears all memory associated with the vector and calls destructors
    auto clear() noexcept -> void {
        for (usize i = 0; i < this->size_; i++) { data()[i].~Item(); }
        this->size_ = 0;
    }

    [[nodiscard]] constexpr auto begin() noexcept -> iterator { return data(); }
    [[nodiscard]] constexpr auto end() noexcept -> iterator { return data() + this->size_; }
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator { return data(); }
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator {
        return data() + this->size_;
    }

    // Ambiguous but required for iterator, only checks for emptiness
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return this->size_ == 0; }
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return this->size_; }

    [[nodiscard]] auto data() noexcept -> Item* { return reinterpret_cast<Item*>(this->items_); }
    [[nodiscard]] auto data() const noexcept -> const Item* {
        return reinterpret_cast<const Item*>(this->items_);
    }

    // ADL dispatcher for copy assignment
    friend auto swap(StaticVector& lhs, StaticVector& rhs) noexcept -> void { lhs.swap(rhs); }

  private:
    // https://en.cppreference.com/cpp/algorithm/swap
    auto swap(StaticVector& other) noexcept -> void {
        if (this == &other) { return; }

        // For trivial types swap the buffers up to the max size used
        if constexpr (TriviallyCopyable<Item>) {
            const usize max_size = std::max(this->size_, other.size_);
            std::swap_ranges(this->items_, this->items_ + (max_size * sizeof(Item)), other.items_);
            std::swap(this->size_, other.size_);
        } else {
            auto& smaller = (this->size_ < other.size_) ? *this : other;
            auto& larger  = (this->size_ < other.size_) ? other : *this;

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
};

} // namespace porpoise

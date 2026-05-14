#pragma once

#include <array>

#include "types.hh"

namespace porpoise::fixed::detail {

// Non-default constructible object use a raw byte array initialized on the fly
template <typename Data, usize Capacity> struct Storage {
    alignas(Data) std::byte items[Capacity * sizeof(Data)];

    [[nodiscard]] auto data() noexcept { return reinterpret_cast<Data*>(items); }
    [[nodiscard]] auto data() const noexcept { return reinterpret_cast<const Data*>(items); }
};

// Default constructible objects can be freely constructed all at once
template <DefaultConstructible Data, usize Capacity> struct Storage<Data, Capacity> {
    std::array<Data, Capacity> items{};

    [[nodiscard]] constexpr auto data() noexcept { return items.data(); }
    [[nodiscard]] constexpr auto data() const noexcept { return items.data(); }
};

} // namespace porpoise::fixed::detail

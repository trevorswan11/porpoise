#pragma once

#include <array>

#include "types.hh"

namespace porpoise::fixed::detail {

// Non-default constructible object use a raw byte array initialized on the fly
template <typename Data, usize Capacity> struct Storage {
    // Operations on this byte array are generally non constexpr-capable
    alignas(Data) std::byte items[Capacity * sizeof(Data)];

    template <typename Self> [[nodiscard]] auto data(this Self&& self) noexcept {
        return reinterpret_cast<
            std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Data*, Data*>>(
            self.items);
    }
};

// Default constructible objects can be freely constructed all at once
template <DefaultConstructible Data, usize Capacity> struct Storage<Data, Capacity> {
    std::array<Data, Capacity> items{};

    template <typename Self> [[nodiscard]] constexpr auto data(this Self&& self) noexcept -> auto* {
        return self.items.data();
    }
};

} // namespace porpoise::fixed::detail

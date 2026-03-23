#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <span>
#include <type_traits>
#include <utility>

#include "optional.hpp"
#include "types.hpp"

namespace porpoise::mem {

// Do not free returned memory directly!
class Arena {
  public:
    static constexpr usize BLOCK_SIZE{64 * 1'024};

    // cppcheck-suppress-begin [unreadVariable, internalAstError]
  public:
    // Asserts that the requested type is at most 64KB
    template <typename T, typename... Args>
        requires(std::is_trivially_destructible_v<T>)
    [[nodiscard]] auto make(Args&&... args) -> NonNull<T> {
        static_assert(sizeof(T) <= BLOCK_SIZE, "Block size cannot fit requested type");
        void* mem = alloc(sizeof(T), alignof(T));
        return new (mem) T{std::forward<Args>(args)...};
    }

    // Asserts that the requested type-count product can fit in 64KB
    template <typename T>
        requires(std::is_trivially_destructible_v<T>)
    [[nodiscard]] auto make_array(usize count) -> std::span<T> {
        static_assert(sizeof(T) <= BLOCK_SIZE, "Block size cannot fit requested type");
        const auto size = sizeof(T) * count;
        assert(size <= BLOCK_SIZE && "Block size cannot fit requested type count");
        void* mem = alloc(size, alignof(T));
        return std::span{new (mem) T[count]{}, count};
    }
    // cppcheck-suppress-end [unreadVariable, internalAstError]

    // Extremely efficient, does not invalidate any allocation until rewritten
    auto reset() noexcept -> void {
        current_block_idx_ = 0;
        current_offset_    = 0;
    }

  private:
    auto alloc(usize size, usize align) -> void*;

  private:
    std::deque<std::array<std::byte, BLOCK_SIZE>> blocks_;
    usize                                         current_block_idx_{0};
    usize                                         current_offset_{0};
};

} // namespace porpoise::mem

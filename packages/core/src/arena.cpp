#include "arena.hpp"

namespace porpoise::mem {

// https://github.com/trevorswan11/porpoise/blob/772707146faa9315c24fb079fd759f3715442db1/old/src/util/arena.c
auto Arena::alloc(usize size, usize align) -> void* {
    if (blocks_.empty()) { blocks_.emplace_back(); }
    while (true) {
        auto&       current_block = blocks_[current_block_idx_];
        auto        raw_addr = reinterpret_cast<uintptr_t>(current_block.data() + current_offset_);
        uintptr_t   aligned_addr = (raw_addr + (align - 1)) & ~(align - 1);
        const usize padding      = aligned_addr - raw_addr;

        if (current_offset_ + padding + size <= BLOCK_SIZE) {
            current_offset_ += padding + size;
            return reinterpret_cast<void*>(aligned_addr);
        }

        // Otherwise a new block needs to be created to house the memory
        current_block_idx_++;
        current_offset_ = 0;
        if (current_block_idx_ >= blocks_.size()) { blocks_.emplace_back(); }
    }
}

} // namespace porpoise::mem

#pragma once

#include "types.hh"

namespace porpoise::tests::helpers {

struct Base {
    virtual ~Base() = default;
    i32 x           = 10;
};

struct Derived : Base {
    i32 y = 20;
};

// Non-thread-safe tracker for memory-critical testing
struct RAIITracker {
    static inline i32 live_count     = 0;
    static inline i32 copy_count     = 0;
    static inline i32 move_count     = 0;
    static inline i32 destruct_count = 0;

    static auto reset() -> void { live_count = copy_count = move_count = destruct_count = 0; }

    RAIITracker() { live_count++; }
    ~RAIITracker() {
        live_count--;
        destruct_count++;
    }

    RAIITracker(const RAIITracker&) {
        live_count++;
        copy_count++;
    }

    auto operator=(const RAIITracker&) -> RAIITracker& {
        copy_count++;
        return *this;
    }

    RAIITracker(RAIITracker&&) noexcept {
        live_count++;
        move_count++;
    }

    auto operator=(RAIITracker&&) noexcept {
        move_count++;
        return *this;
    }
};

} // namespace porpoise::tests::helpers

#pragma once

#include <array>
#include <bit>
#include <functional>

#include "hash.hh"
#include "math.hh"
#include "types.hh"

namespace porpoise {

namespace detail {

class Metadata {
  public:
    enum class Fingerprint : u8 {
        OPEN      = 0,
        TOMBSTONE = 1,
    };

  public:
    constexpr Metadata() noexcept = default;
    constexpr Metadata(u8 fingerprint, bool used) noexcept {
        raw_ |= used << USED_OFFSET;
        raw_ |= fingerprint & FINGERPRINT_MASK;
    }

    constexpr Metadata(Fingerprint fingerprint, bool used) noexcept
        : Metadata{static_cast<u8>(fingerprint), used} {}

    [[nodiscard]] constexpr auto is_used() const noexcept -> bool { return (raw_ & USED_MASK) > 0; }
    [[nodiscard]] constexpr auto get_fingerprint() const noexcept -> u8 {
        return raw_ & FINGERPRINT_MASK;
    }

    constexpr auto               open_up() noexcept -> void { *this = {Fingerprint::OPEN, false}; }
    [[nodiscard]] constexpr auto is_open() const noexcept -> bool {
        return *this == Metadata{Fingerprint::OPEN, false};
    }

    constexpr auto bury() noexcept -> void { *this = {Fingerprint::TOMBSTONE, false}; }
    [[nodiscard]] constexpr auto is_tombstone() const noexcept -> bool {
        return *this == Metadata{Fingerprint::TOMBSTONE, false};
    }

    // Sets the inner fingerprint and marks the metadata as used
    constexpr auto fill(u8 fingerprint) noexcept -> void { *this = {fingerprint, true}; }

    // Only the 7 most significant bits of the result are relevant
    static auto take_fingerprint(u64 hash) noexcept -> u8 {
        return FINGERPRINT_MASK & (hash >> (64 - USED_OFFSET));
    }

    [[nodiscard]] auto operator==(const Metadata&) const noexcept -> bool = default;

  private:
    static constexpr u8 FINGERPRINT_MASK = 0x7F;
    static constexpr u8 USED_MASK        = 0x80;
    static constexpr u8 USED_OFFSET      = std::countr_zero(USED_MASK);

  private:
    u8 raw_{0};
};

// A fixed-size zero-allocation container supporting hash-map operations
template <typename Key, typename Value, usize Capacity, typename Hash, typename Equal>
class StaticHashMap {
    static_assert(is_power_of_two(Capacity), "HashMap capacity must be a power of two");

  public:
    constexpr StaticHashMap() noexcept = default;
    constexpr ~StaticHashMap() { clear(); }
    constexpr ~StaticHashMap()
        requires(TriviallyDestructible<Key>)
    = default;

    [[nodiscard]] constexpr auto size() const noexcept -> usize { return size_; }
    [[nodiscard]] constexpr auto capacity() const noexcept -> usize { return Capacity; }

    [[nodiscard]] constexpr auto key_data() noexcept -> Key* {
        return reinterpret_cast<Key*>(keys_);
    }

    [[nodiscard]] constexpr auto key_data() const noexcept -> Key* {
        return reinterpret_cast<const Key*>(keys_);
    }

    [[nodiscard]] constexpr auto value_data() noexcept -> Value* {
        return reinterpret_cast<Value*>(values_);
    }

    [[nodiscard]] constexpr auto value_data() const noexcept -> Value* {
        return reinterpret_cast<const Value*>(values_);
    }

    constexpr auto clear() noexcept -> void {
        if constexpr (!TriviallyDestructible<Key> || !TriviallyDestructible<Value>) {
            for (usize i = 0; i < Capacity; ++i) {
                if (metadata_[i].is_used()) {
                    if constexpr (!TriviallyDestructible<Key>) { key_data()[i].~Key(); }
                    if constexpr (!TriviallyDestructible<Value>) { value_data()[i].~Value(); }
                }
            }
        }

        metadata_.fill(Metadata{});
        size_ = 0;
    }

  private:
    std::array<Metadata, Capacity> metadata_{};
    alignas(Key) std::byte keys_[Capacity * sizeof(Key)];
    alignas(Value) std::byte values_[Capacity * sizeof(Value)];
    usize size_{0};
};

} // namespace detail

template <typename Key,
          typename Value,
          usize Capacity,
          typename Hash    = hash::Hash<Key>,
          typename Compare = std::equal_to<Key>>
using StaticHashMap = detail::StaticHashMap<Key, Value, ceil_power_of_two(Capacity), Hash, Compare>;

} // namespace porpoise

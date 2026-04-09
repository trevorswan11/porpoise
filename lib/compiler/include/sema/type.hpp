#pragma once

#include <cassert>
#include <span>
#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "array.hpp"
#include "hash.hpp"
#include "optional.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise::sema {

enum class TypeKind : u8 {
    INT,
    LONG,
    SIZE,
    UINT,
    ULONG,
    USIZE,
    BYTE,
    BOOL,
    FLOAT,
    DOUBLE,
    VOID,
    TYPE,
    SLICE,
    ARRAY,
    POINTER,
    REFERENCE,
    ENUM,
    STRUCT,
    UNION,
    FUNCTION,
    TEST_BLOCK,
};

class Type;

namespace types {

using PrimitiveType = std::monostate;

struct Slice {
    NonNull<Type> underlying;
    bool          null_terminated;
};

struct Array {
    NonNull<Type> underlying;
    usize         len;
    bool          null_terminated;
};

struct Pointer {
    NonNull<Type> underlying;
};

struct Reference {
    NonNull<Type> underlying;
};

struct Enum {
    NonNull<Type> underlying;
};

struct Struct {
    bool packed;
};

struct Function {
    std::span<NonNull<Type>> params;
    NonNull<Type>            return_type;
};

template <typename T>
concept KeyMarker = std::is_convertible_v<T, uptr> && std::is_convertible_v<T, uptr>;

class Key {
  public:
    template <KeyMarker A = uptr, KeyMarker B = uptr>
    Key(TypeKind kind,
        bool     mut,
        usize    idx      = 0,
        A        marker_a = 0,
        B        marker_b = 0,
        bool     flag     = false) noexcept
        : kind_{kind}, mut_{mut}, idx_{idx}, marker_a_{static_cast<uptr>(marker_a)},
          marker_b_{static_cast<uptr>(marker_b)}, flag_{flag} {}

    MAKE_GETTER(kind, TypeKind)

    // This is a high quality hash for the purposes of `unordered_dense`
    [[nodiscard]] auto hash() const noexcept -> u64 {
        hash::Hasher h{std::to_underlying(kind_)};
        h.combine(mut_);
        h.combine(idx_);
        h.combine(marker_a_);
        h.combine(marker_b_);
        h.combine(flag_);
        return h.finalize();
    }

    auto                        emplace_idx(usize idx) noexcept -> void { idx_ = idx; }
    template <KeyMarker T> auto emplace_marker_a(T marker_a) noexcept -> void {
        marker_a_ = static_cast<uptr>(marker_a);
    }

    template <KeyMarker T> auto emplace_marker_b(T marker_b) noexcept -> void {
        marker_b_ = static_cast<uptr>(marker_b);
    }
    auto emplace_flag(bool flag) noexcept -> void { flag_ = flag; }

    bool operator==(const Key&) const noexcept = default;

  private:
    TypeKind kind_;
    bool     mut_;
    usize    idx_;
    uptr     marker_a_;
    uptr     marker_b_;
    bool     flag_;
};

} // namespace types

class Type {
  public:
    using Resolved = std::variant<types::PrimitiveType,
                                  types::Slice,
                                  types::Array,
                                  types::Pointer,
                                  types::Reference,
                                  types::Enum,
                                  types::Struct,
                                  types::Function>;

  public:
    explicit Type(TypeKind kind) noexcept : kind_{kind} {}
    ~Type() = default;

    Type(const Type&)                    = delete;
    auto operator=(const Type&) -> Type& = delete;
    Type(Type&&) noexcept                = delete;
    auto operator=(Type&&) -> Type&      = delete;

    MAKE_GETTER(kind, TypeKind)
    MAKE_OPTIONAL_UNPACKER(resolved, Resolved, resolved_, *)

    template <typename Self, typename T> [[nodiscard]] auto as(this Self&& self) -> auto& {
        return std::get<T>(self.resolved_.value());
    }

    template <typename Self, typename T> [[nodiscard]] auto as_opt(this Self&& self) noexcept {
        if (!self.resolved_) { return std::nullopt; }
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              Optional<const T&>,
                                              Optional<T&>>;

        if (!std::holds_alternative<T>(*self.resolved_)) { return ReturnType{std::nullopt}; }
        return ReturnType{std::get<T>(*self.resolved_)};
    }

    // Intended for use on pass 1 only
    constexpr auto set_symbol_table(usize idx) noexcept -> void {
        assert(idx != array::SENTINEL_IDX && "Attempt to set sentinel index");
        scope_table_idx_ = idx;
    }

    [[nodiscard]] constexpr auto has_symbol_table() const noexcept -> bool {
        return scope_table_idx_ != array::SENTINEL_IDX;
    }

    auto resolve(Resolved type) noexcept -> void { resolved_.emplace(std::move(type)); }

  private:
    TypeKind           kind_;
    usize              scope_table_idx_{array::SENTINEL_IDX};
    Optional<Resolved> resolved_;
};

static_assert(std::is_trivially_destructible_v<Type>);

} // namespace porpoise::sema

// https://github.com/martinus/unordered_dense?tab=readme-ov-file#323-specialize-ankerlunordered_densehash
template <> struct ankerl::unordered_dense::hash<porpoise::sema::types::Key> {
    using is_avalanching = void;
    using Key            = porpoise::sema::types::Key;
    [[nodiscard]] auto operator()(const Key& key) const noexcept { return key.hash(); }
};

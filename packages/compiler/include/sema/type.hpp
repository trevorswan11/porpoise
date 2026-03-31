#pragma once

#include <span>
#include <type_traits>
#include <utility>

#include <ankerl/unordered_dense.h>

#include "common.hpp"
#include "hash.hpp"
#include "optional.hpp"
#include "types.hpp"
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
    usize         variant_table_idx;
};

struct Struct {
    usize body_table_idx;
    bool  packed;
};

struct Union {
    usize field_table_idx;
};

struct Function {
    std::span<NonNull<Type>> params;
    NonNull<Type>            return_type;
};

class Key {
  public:
    template <typename A = uptr, typename B = uptr>
        requires(std::is_convertible_v<A, uptr> && std::is_convertible_v<B, uptr>)
    Key(TypeKind kind,
        bool     mut,
        usize    idx      = 0,
        A        marker_a = 0,
        B        marker_b = 0,
        bool     flag     = false) noexcept
        : kind_{kind}, mut_{mut}, idx_{idx}, marker_a_{static_cast<uptr>(marker_a)},
          marker_b_{static_cast<uptr>(marker_b)}, flag_{flag} {}

    MAKE_GETTER(kind, TypeKind)

    [[nodiscard]] auto hash() const noexcept -> u64 {
        hash::Hasher h{std::to_underlying(kind_)};
        h.combine(mut_);
        h.combine(idx_);
        h.combine(marker_a_);
        h.combine(marker_b_);
        h.combine(flag_);
        return h.finalize();
    }

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
                                  types::Union,
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

    template <typename T> [[nodiscard]] auto as() const -> const T& {
        return std::get<T>(resolved_.value());
    }

    template <typename T> [[nodiscard]] auto as_opt() const noexcept -> Optional<const T&> {
        if (!resolved_) { return std::nullopt; }
        if (!std::holds_alternative<T>(*resolved_)) { return std::nullopt; }
        return std::get<T>(*resolved_);
    }

    auto resolve(Resolved type) noexcept -> void { resolved_.emplace(std::move(type)); }

  private:
    TypeKind           kind_;
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

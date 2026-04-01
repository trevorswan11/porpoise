#pragma once

#include <cassert>
#include <span>
#include <type_traits>
#include <utility>

#include <ankerl/unordered_dense.h>

#include "array.hpp"
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
};

struct Struct {
    bool packed;
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

    // Should be populated on pass 1 when used
    struct Metadata {
        constexpr Metadata(usize scope_idx, usize param_idx) noexcept
            : scope_table_idx{scope_idx}, parameter_table_idx{param_idx} {
            assert(scope_table_idx != array::SENTINEL_IDX && "Illegal scope table index");
        }

        usize scope_table_idx;
        usize parameter_table_idx;
    };

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

    // The scope table should be valid (i.e. not a sentinel index)
    constexpr auto set_metadata(usize           scope_table_idx,
                                Optional<usize> parameter_table_idx = std::nullopt) noexcept
        -> void {
        metadata_.emplace(scope_table_idx, parameter_table_idx.value_or(array::SENTINEL_IDX));
    }

    [[nodiscard]] constexpr auto has_parameter_table() const noexcept -> bool {
        return metadata_ && metadata_->parameter_table_idx != array::SENTINEL_IDX;
    }

    auto resolve(Resolved type) noexcept -> void { resolved_.emplace(std::move(type)); }

  private:
    TypeKind           kind_;
    Optional<Metadata> metadata_;
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

#pragma once

#include <span>
#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "sema/symbol.hpp"

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
    SymbolTable   variants;
};

struct Struct {
    SymbolTable body;
    bool        packed;
};

struct Union {
    SymbolTable fields;
};

struct Function {
    std::span<NonNull<Type>> params;
    NonNull<Type>            return_type;
};

class Key {
  public:
    template <typename A = uintptr_t, typename B = uintptr_t>
        requires(std::is_convertible_v<A, uintptr_t> && std::is_convertible_v<B, uintptr_t>)
    Key(TypeKind kind, bool mut, A marker_a = 0, B marker_b = 0, bool flag = false) noexcept
        : kind_{kind}, mut_{mut}, marker_a_{static_cast<uintptr_t>(marker_a)},
          marker_b_{static_cast<uintptr_t>(marker_b)}, flag_{flag} {}

    [[nodiscard]] auto hash() const noexcept -> u64 {
        auto h = porpoise::hash::wyhash::hash(static_cast<u64>(kind_));
        porpoise::hash::combine(h, mut_);
        porpoise::hash::combine(h, marker_a_);
        porpoise::hash::combine(h, marker_b_);
        porpoise::hash::combine(h, flag_);
        return h;
    }

    bool operator==(const Key&) const noexcept = default;

  private:
    TypeKind  kind_;
    bool      mut_;
    uintptr_t marker_a_;
    uintptr_t marker_b_;
    bool      flag_;
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

    MAKE_GETTER(kind, TypeKind)
    MAKE_OPTIONAL_UNPACKER(resolved, Resolved, resolved_, *)

    template <typename T> [[nodiscard]] auto as() const noexcept -> Optional<const T&> {
        if (!resolved_) { return std::nullopt; }
        return std::get<T>(*resolved_);
    }

    auto resolve(Resolved type) noexcept -> void { resolved_.emplace(std::move(type)); }

  private:
    TypeKind           kind_;
    Optional<Resolved> resolved_;
};

} // namespace porpoise::sema

// https://github.com/martinus/unordered_dense?tab=readme-ov-file#323-specialize-ankerlunordered_densehash
template <> struct ankerl::unordered_dense::hash<porpoise::sema::types::Key> {
    using is_avalanching = void;
    using Key            = porpoise::sema::types::Key;
    [[nodiscard]] auto operator()(const Key& key) const noexcept { return key.hash(); }
};

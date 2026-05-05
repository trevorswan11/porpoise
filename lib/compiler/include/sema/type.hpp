#pragma once

#include <cassert>
#include <span>
#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "hash.hpp"
#include "memory.hpp"
#include "option.hpp"
#include "static_vector.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise::sema {

enum class TypeKind : u8 {
    POISON,
    I32,
    I64,
    ISIZE,
    U32,
    U64,
    USIZE,
    U8,
    BOOL,
    F32,
    F64,
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
    BLOCK,
    MATCH_ARM,
    MODULE,
};

class Type;

namespace types {

struct Poison {};

using PrimitiveType = Unit;

struct Slice {
    Type& underlying;
    bool  null_terminated;
};

struct Array {
    Type& underlying;
    usize len;
    bool  null_terminated;
};

struct Pointer {
    Type& underlying;
};

struct Reference {
    Type& underlying;
};

struct Enum {
    Type&                         underlying;
    std::span<mem::NonNull<Type>> members;
};

struct Union {
    std::span<mem::NonNull<Type>> fields;
    std::span<mem::NonNull<Type>> members;
};

struct Struct {
    bool                          packed;
    std::span<mem::NonNull<Type>> members;
};

struct Function {
    std::span<mem::NonNull<Type>> params;
    Type&                         return_type;
};

constexpr usize MAX_BUILTIN_PARAMS{4};

struct BuiltinFunction {
    StaticVector<mem::NonNull<Type>, MAX_BUILTIN_PARAMS> params;
    Type&                                                return_type;
};

class Key {
  public:
    template <hash::Hashable... Markers>
    Key(TypeKind kind, bool mut, usize idx = 0, bool flag = false, Markers&&... markers) noexcept
        : kind_{kind}, mut_{mut}, idx_{idx}, flag_{flag} {
        (..., markers_.combine(markers));
    }

    MAKE_GETTER(kind, TypeKind)

    // This is a high quality hash for the purposes of `unordered_dense`
    [[nodiscard]] auto hash() const noexcept -> u64 {
        hash::Hasher h{std::to_underlying(kind_)};
        h.combine(mut_);
        h.combine(idx_);
        h.combine(flag_);
        h.combine(markers_);
        return h.finalize();
    }

    auto                             emplace_idx(usize idx) noexcept -> void { idx_ = idx; }
    auto                             emplace_flag(bool flag) noexcept -> void { flag_ = flag; }
    template <hash::Hashable H> auto emplace_marker(const H& marker) noexcept -> void {
        markers_.combine(marker);
    }

    bool operator==(const Key&) const noexcept = default;

  private:
    TypeKind     kind_;
    bool         mut_;
    usize        idx_;
    bool         flag_;
    hash::Hasher markers_;
};

} // namespace types

class Type {
  public:
    using Resolved = std::variant<types::Poison,
                                  types::PrimitiveType,
                                  types::Slice,
                                  types::Array,
                                  types::Pointer,
                                  types::Reference,
                                  types::Enum,
                                  types::Struct,
                                  types::Function,
                                  types::BuiltinFunction>;

  public:
    explicit Type(TypeKind kind) noexcept : kind_{kind} {}
    ~Type() = default;

    Type(const Type&)                    = delete;
    auto operator=(const Type&) -> Type& = delete;
    Type(Type&&) noexcept                = delete;
    auto operator=(Type&&) -> Type&      = delete;

    MAKE_GETTER(kind, TypeKind)
    MAKE_OPTIONAL_UNPACKER(resolved, Resolved, resolved_, *)

    // Unpacks T from the resolved type assuming the type has been resolved to T
    template <typename T, typename Self> [[nodiscard]] auto as(this Self&& self) -> auto& {
        return std::get<T>(self.resolved_.value());
    }

    // Tries to unpack T, returning an empty option instead of throwing an exception
    template <typename T, typename Self> [[nodiscard]] auto as_opt(this Self&& self) noexcept {
        using ReturnType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                              opt::Option<const T&>,
                                              opt::Option<T&>>;

        if (!self.resolved_ || !std::holds_alternative<T>(*self.resolved_)) {
            return ReturnType{opt::none};
        }
        return ReturnType{std::get<T>(*self.resolved_)};
    }

    // Intended for use on pass 1 only
    constexpr auto set_symbol_table_idx(usize idx) noexcept -> void {
        scope_table_idx_.emplace(idx);
    }

    [[nodiscard]] constexpr auto has_symbol_table_idx() const noexcept -> bool {
        return scope_table_idx_.has_value();
    }

    [[nodiscard]] constexpr auto get_symbol_table_idx() const noexcept -> usize {
        return *scope_table_idx_;
    }

    template <typename Resolvee, typename... Args> auto resolve(Args&&... args) noexcept -> void {
        resolved_.emplace(Resolvee{std::forward<Args>(args)...});
    }

    // Returns the memory address of the Type for a Key's marker
    [[nodiscard]] constexpr auto as_marker() const noexcept -> u64 {
        return reinterpret_cast<u64>(this);
    }

    [[nodiscard]] constexpr auto is_poison() const noexcept -> bool {
        return kind_ == TypeKind::POISON;
    }

  private:
    TypeKind              kind_;
    opt::Index            scope_table_idx_;
    opt::Option<Resolved> resolved_;
};

static_assert(TriviallyDestructible<Type>);

} // namespace porpoise::sema

template <> struct ankerl::unordered_dense::hash<porpoise::sema::types::Key> {
    using is_avalanching = void;
    using Key            = porpoise::sema::types::Key;
    [[nodiscard]] auto operator()(const Key& key) const noexcept { return key.hash(); }
};

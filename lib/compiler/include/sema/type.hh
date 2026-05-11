#pragma once

#include <span>
#include <type_traits>

#include <ankerl/unordered_dense.h>

#include "arena.hh"
#include "hash.hh"
#include "memory.hh"
#include "option.hh"
#include "static_vector.hh"
#include "types.hh"
#include "utility.hh"
#include "variant.hh"

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
    LABEL,
    BLOCK,
    MATCH_ARM,
    MODULE,

    AUTO,
    OPAQUE,
    NORETURN,
};

class Type;

namespace types {

struct Poison {};

using BuiltinType = Unit;

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
    std::span<mem::NonNull<Type>> members;
};

struct Function {
    std::span<mem::NonNull<Type>> params;
    Type&                         return_type;
};

constexpr usize MAX_BUILTIN_PARAMS{4};
using BuiltinParams = StaticVector<mem::NonNull<Type>, MAX_BUILTIN_PARAMS>;

struct BuiltinFunction {
    BuiltinParams params;
    Type&         return_type;
};

class Key {
  public:
    enum class Mutability : u8 {
        MUTABLE,
        IMMUTABLE,
    };

  public:
    template <typename... Markers>
    constexpr Key(TypeKind kind, Mutability mut, Markers&&... markers) noexcept
        : kind_{kind}, mut_{mut} {
        (..., markers_.combine(markers));
    }

    MAKE_GETTER(kind, TypeKind)

    // This is a high quality hash for the purposes of `unordered_dense`
    [[nodiscard]] constexpr auto hash() const noexcept -> u64 {
        hash::Hasher h{std::to_underlying(kind_)};
        h.combine(mut_);
        h.combine(markers_);
        return h.finalize();
    }

    // Emplace a hashable marker into the accumulated markers
    template <typename Marker> constexpr auto imprint(const Marker& marker) noexcept -> void {
        markers_.combine(marker);
    }

    [[nodiscard]] constexpr auto operator==(const Key&) const noexcept -> bool = default;

  private:
    TypeKind     kind_;
    Mutability   mut_;
    hash::Hasher markers_;
};

namespace mut {

constexpr auto MUTABLE   = types::Key::Mutability::MUTABLE;
constexpr auto IMMUTABLE = types::Key::Mutability::IMMUTABLE;

} // namespace mut

} // namespace types

} // namespace porpoise::sema

template <> struct ankerl::unordered_dense::hash<porpoise::sema::types::Key> {
    using is_avalanching = void;
    using Key            = porpoise::sema::types::Key;
    [[nodiscard]] auto operator()(const Key& key) const noexcept { return key.hash(); }
};

namespace porpoise::sema {

// A semantic type that is entirely owned by an arena of types
class Type {
  public:
    using Resolved = std::variant<types::Poison,
                                  types::BuiltinType,
                                  types::Slice,
                                  types::Array,
                                  types::Pointer,
                                  types::Reference,
                                  types::Enum,
                                  types::Struct,
                                  types::Function,
                                  types::BuiltinFunction>;

  public:
    ~Type() = default;

    Type(const Type&)                    = delete;
    auto operator=(const Type&) -> Type& = delete;
    Type(Type&&) noexcept                = delete;
    auto operator=(Type&&) -> Type&      = delete;

    MAKE_GETTER(kind, TypeKind)
    MAKE_OPTIONAL_UNPACKER(resolved, const Resolved&, resolved_, *)

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
        symbol_table_idx_.emplace(idx);
    }

    MAKE_OPTIONAL_UNPACKER(symbol_table_idx, usize, symbol_table_idx_, *)

    template <typename Resolvee, typename... Args> auto resolve(Args&&... args) noexcept -> void {
        resolved_.emplace(Resolvee{std::forward<Args>(args)...});
    }

    // Returns the memory address of the Type for a Key's hash
    [[nodiscard]] constexpr auto hash() const noexcept -> u64 {
        return reinterpret_cast<u64>(this);
    }

    [[nodiscard]] constexpr auto is_poison() const noexcept -> bool {
        return kind_ == TypeKind::POISON;
    }

  private:
    explicit Type(TypeKind kind) noexcept : kind_{kind} {}

  private:
    TypeKind              kind_;
    opt::Index            symbol_table_idx_;
    opt::Option<Resolved> resolved_;

    // Initialization is restricted to the pool's arena exclusively
    friend class mem::Arena;
};

static_assert(TriviallyDestructible<Type>);

// All associated type lifetimes are tied to the pool
class TypePool {
  public:
    TypePool() noexcept = default;
    ~TypePool()         = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TypePool)

    // Gets a type by its key or emplace's it into the internal cache
    [[nodiscard]] auto operator[](const types::Key& key) -> Type& { return get_or_emplace(key); }
    [[nodiscard]] auto get_opt(const types::Key& key) noexcept -> opt::Option<Type&>;

  private:
    auto get_or_emplace(const types::Key& key) -> Type&;

  private:
    mem::Arena                                      arena_;
    ankerl::unordered_dense::map<types::Key, Type*> cache_;
};

} // namespace porpoise::sema

template <> struct ankerl::unordered_dense::hash<porpoise::sema::Type> {
    using is_avalanching = void;
    using Type           = porpoise::sema::Type;
    [[nodiscard]] auto operator()(const Type& type) const noexcept { return type.hash(); }
};

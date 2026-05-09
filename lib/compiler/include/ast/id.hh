#pragma once

#include <bit>
#include <concepts>
#include <limits>

#include <fmt/format.h>

#include "syntax/token.hh"

#include "assert.hh"
#include "option.hh"
#include "types.hh"

namespace porpoise {

namespace ast {

namespace detail { constexpr u64 INVALID_ID = std::numeric_limits<u64>::max(); } // namespace detail

enum class NodeKind : u8 {
    ARRAY_EXPRESSION,
    CALL_EXPRESSION,
    DO_WHILE_LOOP_EXPRESSION,
    ENUM_EXPRESSION,
    FOR_LOOP_EXPRESSION,
    FUNCTION_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    IF_EXPRESSION,
    INDEX_EXPRESSION,
    INFINITE_LOOP_EXPRESSION,
    ASSIGNMENT_EXPRESSION,
    BINARY_EXPRESSION,
    DOT_EXPRESSION,
    RANGE_EXPRESSION,
    INITIALIZER_EXPRESSION,
    LABEL_EXPRESSION,
    MATCH_EXPRESSION,
    UNARY_EXPRESSION,
    REFERENCE_EXPRESSION,
    DEREFERENCE_EXPRESSION,
    IMPLICIT_ACCESS_EXPRESSION,
    STRING_EXPRESSION,
    I32_EXPRESSION,
    I64_EXPRESSION,
    ISIZE_EXPRESSION,
    U32_EXPRESSION,
    U64_EXPRESSION,
    USIZE_EXPRESSION,
    U8_EXPRESSION,
    F32_EXPRESSION,
    F64_EXPRESSION,
    BOOL_EXPRESSION,
    VOID_EXPRESSION,
    SCOPE_RESOLUTION_EXPRESSION,
    STRUCT_EXPRESSION,
    TYPE_EXPRESSION,
    UNION_EXPRESSION,
    WHILE_LOOP_EXPRESSION,

    BLOCK_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    DECL_STATEMENT,
    DEFER_STATEMENT,
    DISCARD_STATEMENT,
    EXPRESSION_STATEMENT,
    IMPORT_STATEMENT,
    RETURN_STATEMENT,
    TEST_STATEMENT,
    USING_STATEMENT,

    DISCARDED, // Represented by Unit
};

namespace traits {

template <typename T> struct NodeKindOf;

template <typename T>
concept ASTNode = requires {
    { NodeKindOf<T>::value() } -> std::same_as<NodeKind>;
};

} // namespace traits

// A compact id for all AST nodes
class NodeID {
  public:
    constexpr NodeID(NodeKind kind, syntax::TokenType type, u64 index) noexcept : raw_{} {
        ASSERT(index <= INDEX_MASK, "Requested node index is too large");
        raw_ |= static_cast<u64>(kind) << KIND_OFFSET;
        raw_ |= static_cast<u64>(type) << TOKEN_TYPE_OFFSET;
        raw_ |= index;
    }

    [[nodiscard]] constexpr auto get_kind() const noexcept -> NodeKind {
        return static_cast<NodeKind>((raw_ & KIND_MASK) >> KIND_OFFSET);
    }

    [[nodiscard]] constexpr auto get_token_type() const noexcept -> syntax::TokenType {
        return static_cast<syntax::TokenType>((raw_ & TOKEN_TYPE_MASK) >> TOKEN_TYPE_OFFSET);
    }

    [[nodiscard]] constexpr auto get_index() const noexcept -> u64 { return raw_ & INDEX_MASK; }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> NodeID {
        return NodeID{detail::INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
        return raw_ != detail::INVALID_ID;
    }

    template <traits::ASTNode N> [[nodiscard]] constexpr auto is() const noexcept -> bool {
        return get_kind() == traits::NodeKindOf<N>::value();
    }

    template <traits::ASTNode... Ns> [[nodiscard]] constexpr auto any() const noexcept -> bool {
        const auto kind = get_kind();
        return ((kind == traits::NodeKindOf<Ns>::value()) || ...);
    }

  private:
    constexpr explicit NodeID(u64 raw) noexcept : raw_{raw} {}

  private:
    static constexpr u64 KIND_MASK         = 0xFF00000000000000ULL;
    static constexpr u64 KIND_OFFSET       = std::countr_zero(KIND_MASK);
    static constexpr u64 TOKEN_TYPE_MASK   = 0x00FF000000000000ULL;
    static constexpr u64 TOKEN_TYPE_OFFSET = std::countr_zero(TOKEN_TYPE_MASK);
    static constexpr u64 INDEX_MASK        = 0x0000FFFFFFFFFFFFULL;

  private:
    u64 raw_;

    template <NodeKind... AllowedKinds> friend class Handle;
};

// A pseudo-type-safe wrapper around nodes, defaulting to an invalid state
template <NodeKind... AllowedKinds> class Handle {
  public:
    constexpr explicit Handle(NodeID id) noexcept : id_{id} {
        ASSERT(id.is_valid(), "Attempt to create Handle from invalid NodeID");
#ifndef NDEBUG
        const auto actual_kind = id.get_kind();
        const auto is_allowed  = ((actual_kind == AllowedKinds) || ...);
        ASSERT(is_allowed, "Assigned invalid NodeKind to Handle");
#endif
    }

    template <NodeKind... OtherKinds>
    constexpr Handle(const Handle<OtherKinds...>& other) noexcept : id_{*other} {
        // Shallow verify at compile time to enforce possible construction
        constexpr auto check_compat   = []<NodeKind K> { return ((K == AllowedKinds) || ...); };
        constexpr auto any_compatible = (check_compat.template operator()<OtherKinds>() || ...);
        static_assert(any_compatible, "No requested kinds are compatible with provided ones");

        // Runtime checks can be more specific to the actual state
        ASSERT(other.is_valid(), "Attempt to create Handle from invalid Handle");
        ASSERT(((other.operator->()->get_kind() == AllowedKinds) || ...),
               "Provided kind does not match");
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> NodeID { return id_; }
    [[nodiscard]] constexpr auto operator->() const noexcept -> const NodeID* { return &id_; }

    [[nodiscard]] constexpr auto        is_valid() const noexcept -> bool { return id_.is_valid(); }
    [[nodiscard]] static constexpr auto make_invalid() noexcept -> Handle {
        return Handle{detail::INVALID_ID};
    }

  private:
    constexpr explicit Handle(u64 raw) noexcept : id_{raw} {}

  private:
    NodeID id_{NodeID::make_invalid()};
};

using ExpressionHandle = Handle<NodeKind::ARRAY_EXPRESSION,
                                NodeKind::ASSIGNMENT_EXPRESSION,
                                NodeKind::BINARY_EXPRESSION,
                                NodeKind::CALL_EXPRESSION,
                                NodeKind::DO_WHILE_LOOP_EXPRESSION,
                                NodeKind::DOT_EXPRESSION,
                                NodeKind::ENUM_EXPRESSION,
                                NodeKind::FOR_LOOP_EXPRESSION,
                                NodeKind::FUNCTION_EXPRESSION,
                                NodeKind::IDENTIFIER_EXPRESSION,
                                NodeKind::IF_EXPRESSION,
                                NodeKind::INDEX_EXPRESSION,
                                NodeKind::INFINITE_LOOP_EXPRESSION,
                                NodeKind::INITIALIZER_EXPRESSION,
                                NodeKind::LABEL_EXPRESSION,
                                NodeKind::MATCH_EXPRESSION,
                                NodeKind::UNARY_EXPRESSION,
                                NodeKind::REFERENCE_EXPRESSION,
                                NodeKind::DEREFERENCE_EXPRESSION,
                                NodeKind::IMPLICIT_ACCESS_EXPRESSION,
                                NodeKind::STRING_EXPRESSION,
                                NodeKind::I32_EXPRESSION,
                                NodeKind::I64_EXPRESSION,
                                NodeKind::ISIZE_EXPRESSION,
                                NodeKind::U32_EXPRESSION,
                                NodeKind::U64_EXPRESSION,
                                NodeKind::USIZE_EXPRESSION,
                                NodeKind::U8_EXPRESSION,
                                NodeKind::F32_EXPRESSION,
                                NodeKind::F64_EXPRESSION,
                                NodeKind::BOOL_EXPRESSION,
                                NodeKind::VOID_EXPRESSION,
                                NodeKind::RANGE_EXPRESSION,
                                NodeKind::SCOPE_RESOLUTION_EXPRESSION,
                                NodeKind::STRUCT_EXPRESSION,
                                NodeKind::TYPE_EXPRESSION,
                                NodeKind::UNION_EXPRESSION,
                                NodeKind::WHILE_LOOP_EXPRESSION>;

using StatementHandle = Handle<NodeKind::BLOCK_STATEMENT,
                               NodeKind::DECL_STATEMENT,
                               NodeKind::DEFER_STATEMENT,
                               NodeKind::DISCARD_STATEMENT,
                               NodeKind::EXPRESSION_STATEMENT,
                               NodeKind::IMPORT_STATEMENT,
                               NodeKind::RETURN_STATEMENT,
                               NodeKind::BREAK_STATEMENT,
                               NodeKind::CONTINUE_STATEMENT,
                               NodeKind::TEST_STATEMENT,
                               NodeKind::USING_STATEMENT>;

#define MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(name, modifier)            \
    [[nodiscard]] constexpr auto is_##name() const noexcept -> bool { \
        if (is_value()) { return false; }                             \
        return underlying_ == modifier;                               \
    }

class TypeModifier {
  public:
    enum class Modifier : u8 {
        VALUE,
        REF,
        MUT_REF,
        PTR,
        MUT_PTR,
        VOLATILE,
    };

  public:
    constexpr TypeModifier() noexcept = default;
    constexpr explicit TypeModifier(Modifier underlying) noexcept : underlying_{underlying} {}
    constexpr explicit TypeModifier(u64 raw) noexcept : underlying_{static_cast<Modifier>(raw)} {}
    explicit TypeModifier(const syntax::Token& tok) noexcept;

    // Whether or not the type is a 'value' type (no modifier), mutually exclusive result.
    [[nodiscard]] constexpr auto is_value() const noexcept -> bool {
        return underlying_ == Modifier::VALUE;
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ref, Modifier::MUT_REF)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ref, Modifier::REF)
    [[nodiscard]] constexpr auto is_ref() const noexcept -> bool {
        return is_mutable_ref() || is_const_ref();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ptr, Modifier::MUT_PTR)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ptr, Modifier::PTR)
    [[nodiscard]] constexpr auto is_ptr() const noexcept -> bool {
        return is_mutable_ptr() || is_const_ptr();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(volatile, Modifier::VOLATILE)

    constexpr friend auto operator==(const TypeModifier& lhs, const TypeModifier& rhs) noexcept
        -> bool {
        return lhs.underlying_ == rhs.underlying_;
    }

    [[nodiscard]] constexpr operator u64() const noexcept { return static_cast<u64>(underlying_); }

  private:
    Modifier underlying_{Modifier::VALUE};

    friend struct fmt::formatter<porpoise::ast::TypeModifier>;
};

namespace type_modifiers {

constexpr ast::TypeModifier VALUE{};
constexpr ast::TypeModifier REF{ast::TypeModifier::Modifier::REF};
constexpr ast::TypeModifier MUT_REF{ast::TypeModifier::Modifier::MUT_REF};
constexpr ast::TypeModifier PTR{ast::TypeModifier::Modifier::PTR};
constexpr ast::TypeModifier MUT_PTR{ast::TypeModifier::Modifier::MUT_PTR};
constexpr ast::TypeModifier VOLATILE{ast::TypeModifier::Modifier::VOLATILE};

} // namespace type_modifiers

enum class ExplicitTypeKind : u8 {
    IDENT,
    SCOPE,
    CALL,
    FUNCTION,
    RECURSIVE,
    STRUCT,
    ENUM,
    UNION,
    ARRAY,
};

namespace traits {

template <typename T> struct ExplicitTypeKindOf;

template <typename T>
concept ASTExplicitType = requires {
    { ExplicitTypeKindOf<T>::value() } -> std::same_as<ExplicitTypeKind>;
};

} // namespace traits

// A compact id for all AST explicit types
class ExplicitTypeID {
  public:
    constexpr ExplicitTypeID(ExplicitTypeKind  kind,
                             TypeModifier      mod,
                             syntax::TokenType token_type,
                             u64               index) noexcept
        : raw_{} {
        static_assert(magic_enum::enum_count<ExplicitTypeKind>() <= 0xF,
                      "ExplicitTypeKind enum too large");
        static_assert(magic_enum::enum_count<TypeModifier::Modifier>() <= 0xF,
                      "TypeModifier enum too large");

        ASSERT(index <= INDEX_MASK, "Requested type index is too large");
        raw_ |= static_cast<u64>(kind) << KIND_OFFSET;
        raw_ |= static_cast<u64>(mod) << MODIFIER_OFFSET;
        raw_ |= static_cast<u64>(token_type) << MODIFIER_OFFSET;
        raw_ |= index;
    }

    [[nodiscard]] constexpr auto get_kind() const noexcept -> ExplicitTypeKind {
        return static_cast<ExplicitTypeKind>((raw_ & KIND_MASK) >> KIND_OFFSET);
    }

    [[nodiscard]] constexpr auto get_modifier() const noexcept -> u64 {
        return TypeModifier{(raw_ & MODIFIER_MASK) >> MODIFIER_OFFSET};
    }

    [[nodiscard]] constexpr auto get_token_type() const noexcept -> syntax::TokenType {
        return static_cast<syntax::TokenType>((raw_ & TOKEN_TYPE_MASK) >> TOKEN_TYPE_OFFSET);
    }

    [[nodiscard]] constexpr auto get_index() const noexcept -> u64 { return raw_ & INDEX_MASK; }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> ExplicitTypeID {
        return ExplicitTypeID{detail::INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
        return raw_ != detail::INVALID_ID;
    }

    template <traits::ASTExplicitType N> [[nodiscard]] constexpr auto is() const noexcept -> bool {
        return get_kind() == traits::ExplicitTypeKindOf<N>::value();
    }

    template <traits::ASTExplicitType... Ns>
    [[nodiscard]] constexpr auto any() const noexcept -> bool {
        const auto kind = get_kind();
        return ((kind == traits::ExplicitTypeKindOf<Ns>::value()) || ...);
    }

  private:
    constexpr explicit ExplicitTypeID(const u64& raw) noexcept : raw_{raw} {}

  private:
    static constexpr u64 KIND_MASK         = 0xF000000000000000ULL;
    static constexpr u64 KIND_OFFSET       = std::countr_zero(KIND_MASK);
    static constexpr u64 MODIFIER_MASK     = 0x0F00000000000000ULL;
    static constexpr u64 MODIFIER_OFFSET   = std::countr_zero(MODIFIER_MASK);
    static constexpr u64 TOKEN_TYPE_MASK   = 0x00FF000000000000ULL;
    static constexpr u64 TOKEN_TYPE_OFFSET = std::countr_zero(TOKEN_TYPE_MASK);
    static constexpr u64 INDEX_MASK        = 0x0000FFFFFFFFFFFFULL;

  private:
    u64 raw_;
};

#undef MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY

} // namespace ast

namespace traits {

template <> struct Nullable<ast::NodeID> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::NodeID {
        return ast::NodeID::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(const ast::NodeID& id) noexcept -> bool {
        return id.is_valid();
    }
};

template <ast::NodeKind... Kinds> struct Nullable<ast::Handle<Kinds...>> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::Handle<Kinds...> {
        return ast::Handle<Kinds...>::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(const ast::Handle<Kinds...>& handle) noexcept
        -> bool {
        return handle.is_valid();
    }
};

template <> struct Nullable<ast::ExplicitTypeID> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::ExplicitTypeID {
        return ast::ExplicitTypeID::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(const ast::ExplicitTypeID& id) noexcept -> bool {
        return id.is_valid();
    }
};

} // namespace traits

} // namespace porpoise

template <> struct fmt::formatter<porpoise::ast::TypeModifier> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::TypeModifier& t, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(t.underlying_));
    }
};

#pragma once

#include <bit>
#include <concepts>
#include <limits>

#include "syntax/token.hh"

#include "assert.hh"
#include "option.hh"
#include "types.hh"

namespace porpoise {

namespace ast {

constexpr u64 INVALID_ID = std::numeric_limits<u64>::max();

enum class NodeKind : u8 {
    DISCARDED,

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
        return NodeID{INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool { return raw_ != INVALID_ID; }

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
    constexpr Handle(const Handle<OtherKinds...>& other) noexcept : id_{other.id()} {
        ASSERT(other->is_valid(), "Attempt to create Handle from invalid Handle");
        constexpr auto check_subset = []<NodeKind K> { return ((K == AllowedKinds) || ...); };
        constexpr auto is_subset    = (check_subset.template operator()<OtherKinds>() && ...);
        static_assert(is_subset, "Source kinds are not a subset");
    }

    [[nodiscard]] constexpr auto get_id() const noexcept -> NodeID { return id_; }
    [[nodiscard]] constexpr auto operator*() const noexcept -> NodeID { return id_; }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> Handle {
        return Handle{INVALID_ID};
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

enum class TypeModifier : u8 {
    REF,
    MUT_REF,
    PTR,
    MUT_PTR,
    VOLATILE,
};

#define MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(name, modifier)            \
    [[nodiscard]] constexpr auto is_##name() const noexcept -> bool { \
        if (is_value()) { return false; }                             \
        return get_modifier<TypeModifier>() == modifier;              \
    }

// A compact id for all AST explicit types
class ExplicitTypeID {
  public:
    constexpr explicit ExplicitTypeID(const opt::Option<TypeModifier>& mod,
                                      const u64&                       index) noexcept
        : raw_{} {
        ASSERT(index <= INDEX_MASK, "Requested node index is too large");
        raw_ |= static_cast<u64>(mod.value_or(0)) << MODIFIER_OFFSET;
        raw_ |= index;
    }

    [[nodiscard]] constexpr auto is_value() const noexcept -> bool {
        return get_modifier<u64>() == 0;
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ref, TypeModifier::MUT_REF)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ref, TypeModifier::REF)
    [[nodiscard]] constexpr auto is_ref() const noexcept -> bool {
        return is_mutable_ref() || is_const_ref();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ptr, TypeModifier::MUT_PTR)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ptr, TypeModifier::PTR)
    [[nodiscard]] constexpr auto is_ptr() const noexcept -> bool {
        return is_mutable_ptr() || is_const_ptr();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(volatile, TypeModifier::VOLATILE)

    [[nodiscard]] constexpr auto get_index() const noexcept -> u64 { return raw_ & INDEX_MASK; }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> ExplicitTypeID {
        return ExplicitTypeID{INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool { return raw_ != INVALID_ID; }

  private:
    constexpr explicit ExplicitTypeID(const u64& raw) noexcept : raw_{raw} {}

    template <typename ReturnType>
    [[nodiscard]] constexpr auto get_modifier() const noexcept -> ReturnType {
        const auto value = (raw_ & MODIFIER_MASK) >> MODIFIER_OFFSET;
        if constexpr (std::is_same_v<ReturnType, u64>) {
            return static_cast<u64>(value);
        } else if constexpr (std::is_same_v<ReturnType, TypeModifier>) {
            ASSERT(value != 0, "Attempt to poll modifier of base type");
            return static_cast<TypeModifier>(value);
        } else {
            static_assert(false, "Invalid modifier return type");
        }
    }

  private:
    static constexpr u64 MODIFIER_MASK   = 0xFF00000000000000ULL;
    static constexpr u64 MODIFIER_OFFSET = std::countr_zero(MODIFIER_MASK);
    static constexpr u64 INDEX_MASK      = 0x00FFFFFFFFFFFFFFULL;

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
        return handle->is_valid();
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

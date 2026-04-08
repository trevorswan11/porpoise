#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <span>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "syntax/token.hpp"

#include "memory.hpp"
#include "optional.hpp"
#include "types.hpp"
#include "utility.hpp"

namespace porpoise {

namespace sema { class Type; } // namespace sema

namespace ast {

class Visitor;

enum class NodeKind : u8 {
    ARRAY_EXPRESSION,
    ASSIGNMENT_EXPRESSION,
    BINARY_EXPRESSION,
    CALL_EXPRESSION,
    DO_WHILE_LOOP_EXPRESSION,
    DOT_EXPRESSION,
    ENUM_EXPRESSION,
    FOR_LOOP_EXPRESSION,
    FUNCTION_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    IF_EXPRESSION,
    INDEX_EXPRESSION,
    INFINITE_LOOP_EXPRESSION,
    INITIALIZER_EXPRESSION,
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
    RANGE_EXPRESSION,
    SCOPE_RESOLUTION_EXPRESSION,
    STRUCT_EXPRESSION,
    TYPE_EXPRESSION,
    UNION_EXPRESSION,
    WHILE_LOOP_EXPRESSION,

    BLOCK_STATEMENT,
    DECL_STATEMENT,
    DEFER_STATEMENT,
    DISCARD_STATEMENT,
    EXPRESSION_STATEMENT,
    IMPORT_STATEMENT,
    JUMP_STATEMENT,
    MODULE_STATEMENT,
    USING_STATEMENT,
};

class Node;

// A type that can be anything in the Node inheritance hierarchy
template <typename N>
concept NodeSubtype = std::derived_from<N, Node>;

// A necessarily instantiable Node, meaning it has a NodeKind marker.
template <typename T>
concept LeafNode = NodeSubtype<T> && requires {
    { T::KIND } -> std::convertible_to<NodeKind>;
};

template <typename T> struct is_leaf_node : std::false_type {};
template <LeafNode T> struct is_leaf_node<T> : std::true_type {};
template <typename T> constexpr bool is_leaf_node_v = is_leaf_node<T>::value;

// Used for types who inherit from a CRTP class, avoid otherwise
template <typename T> struct disable_default_parse : std::false_type {};

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = default;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Node)

    virtual auto accept(Visitor& v) const -> void = 0;

    auto get_token() const noexcept -> const syntax::Token& { return start_token_; }
    auto get_kind() const noexcept -> NodeKind { return kind_; }

    [[nodiscard]] auto has_sema_type() const noexcept -> bool { return sema_type_.has_value(); }
    [[nodiscard]] auto get_sema_type() const noexcept -> sema::Type& { return *sema_type_; }
    auto set_sema_type(sema::Type& type) const noexcept -> void { sema_type_.emplace(type); }

    // Compares two nodes at the AST level, ignoring semantic differences.
    friend auto operator==(const Node& lhs, const Node& rhs) noexcept -> bool {
        if (lhs.kind_ != rhs.kind_) { return false; }
        if (lhs.start_token_.type != rhs.start_token_.type) { return false; }
        if (lhs.start_token_.slice != rhs.start_token_.slice) { return false; }
        return lhs.is_equal(rhs);
    }

    template <LeafNode T> auto     is() const noexcept -> bool { return kind_ == T::KIND; }
    template <LeafNode... Ts> auto any() const noexcept -> bool {
        return ((kind_ == Ts::KIND) || ...);
    }

    // A 'safe' alternative to a raw static cast for nodes. Assertion > UB
    template <LeafNode T> [[nodiscard]] static auto as(const Node& n) noexcept -> const T& {
        assert(n.is<T>());
        return static_cast<const T&>(n);
    }

    // Transfers ownership and downcasts a boxed node into the requested type.
    template <LeafNode To, NodeSubtype From>
    static auto downcast(mem::Box<From>&& from) -> mem::Box<To> {
        assert(from && from->template is<To>());
        return mem::box_into<To>(std::move(from));
    }

  protected:
    Node(const syntax::Token& tok, NodeKind kind) noexcept : start_token_{tok}, kind_{kind} {}

    virtual auto is_equal(const Node& other) const noexcept -> bool = 0;

  protected:
    const syntax::Token           start_token_;
    const NodeKind                kind_;
    mutable Optional<sema::Type&> sema_type_;
    friend class ExplicitType;
};

using AST     = std::vector<mem::Box<Node>>;
using ASTView = std::span<const mem::Box<Node>>;

template <typename Derived, typename Base> class NodeBase : public Base {
  protected:
    explicit NodeBase(const syntax::Token& tok) noexcept : Base{tok, Derived::KIND} {}
};

class Expression : public Node {
  protected:
    using Node::Node;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(Expression)

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

template <typename Derived> class ExprBase : public NodeBase<Derived, Expression> {
  protected:
    using NodeBase<Derived, Expression>::NodeBase;
};

class Statement : public Node {
  protected:
    using Node::Node;
    MAKE_MOVE_CONSTRUCTABLE_ONLY(Statement)

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

template <typename Derived> class StmtBase : public NodeBase<Derived, Statement> {
  protected:
    using NodeBase<Derived, Statement>::NodeBase;
};

class DeclStatement;
using Members         = std::vector<mem::Box<DeclStatement>>;
using MembersView     = std::span<const mem::Box<DeclStatement>>;
using MemberValidator = bool(const DeclStatement&);

} // namespace ast

} // namespace porpoise

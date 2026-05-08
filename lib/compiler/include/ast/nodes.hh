#pragma once

#include <vector>

#include "ast/id.hh"

#include "utility.hh"
#include "variant.hh"

namespace porpoise {

namespace syntax { class Parser; } // namespace syntax

namespace ast {

using Members = std::vector<NodeID>;

struct ArrayExpression {
    opt::Option<NodeID> size;
    bool                null_terminated;
    ExplicitTypeID      item_explicit_type;
    std::vector<NodeID> items;
};

struct CallExpression {
    class Argument {
      public:
        explicit Argument(NodeID id) noexcept : argument_{id} {}
        explicit Argument(ExplicitTypeID id) noexcept : argument_{id} {}

        MAKE_VARIANT_UNPACKER(expression, NodeID, NodeID, argument_, std::get)
        MAKE_VARIANT_UNPACKER(type, ExplicitTypeID, ExplicitTypeID, argument_, std::get)
        MAKE_VARIANT_MATCHER(argument_)

      private:
        std::variant<NodeID, ExplicitTypeID> argument_;
    };

    NodeID                function;
    std::vector<Argument> arguments;
};

struct DoWhileLoopExpression {
    NodeID block;
    NodeID condition;
};

struct EnumExpression {
    using Enumeration = std::pair<NodeID, opt::Option<NodeID>>;

    opt::Option<NodeID>      underlying;
    std::vector<Enumeration> enumeration;
    Members                  members;
};

struct ForLoopExpression {
    struct Capture {
        opt::Option<TypeModifier> modifier;
        NodeID                    payload;
    };

    std::vector<NodeID>  iterables;
    std::vector<Capture> captures;
    NodeID               block;
    opt::Option<NodeID>  non_break;
};

struct FunctionExpression {
    struct Self {
        opt::Option<TypeModifier> modifier;
        NodeID                    ident;
    };

    struct Parameter {
        opt::Option<NodeID> ident;
        ExplicitTypeID      explicit_type;
    };

    Self                   self;
    std::vector<Parameter> parameters;
    bool                   variadic;
    ExplicitTypeID         explicit_return_type;
    opt::Option<NodeID>    body;
};

struct GroupExpression {};

struct IdentifierExpression {};

struct IfExpression {
    bool   constexpr_condition;
    NodeID condition;
    NodeID consequence;
    NodeID alternate;
};

struct IndexExpression {
    NodeID array;
    NodeID index;
};

struct InfiniteLoopExpression {
    NodeID block;
};

#define DECLARE_INFIX_EXPRESSION(Type) \
    struct Type {                      \
        NodeID            lhs;         \
        syntax::TokenType op;          \
        NodeID            rhs;         \
    };

DECLARE_INFIX_EXPRESSION(AssignmentExpression)
DECLARE_INFIX_EXPRESSION(BinaryExpression)
DECLARE_INFIX_EXPRESSION(DotExpression)
DECLARE_INFIX_EXPRESSION(RangeExpression)

#undef DECLARE_INFIX_EXPRESSION

struct InitializerExpression {
    struct Initializer {
        NodeID member;
        NodeID value;
    };

    opt::Option<NodeID>      object_type;
    std::vector<Initializer> initializers;
};

struct LabelExpression {
    NodeID name;
    NodeID body;
};

struct MatchExpression {
    struct Arm {
        NodeID              pattern;
        opt::Option<NodeID> capture;
        NodeID              dispatch;
    };

    NodeID              matcher;
    std::vector<Arm>    arms;
    opt::Option<NodeID> catch_all;
};

#define DECLARE_PREFIX_EXPRESSION(Type) \
    struct Type {                       \
        NodeID rhs;                     \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression)
DECLARE_PREFIX_EXPRESSION(ImplicitAccessExpression)

#undef DECLARE_PREFIX_EXPRESSION

template <typename ValueType> struct PrimitiveExpression {
    ValueType value;
};

struct StringExpression : private PrimitiveExpression<std::string> {};

struct I32Expression : private PrimitiveExpression<i32> {};

struct I64Expression : private PrimitiveExpression<i64> {};

struct ISizeExpression : private PrimitiveExpression<isize> {};

struct U32Expression : private PrimitiveExpression<u32> {};

struct U64Expression : private PrimitiveExpression<u64> {};

struct USizeExpression : private PrimitiveExpression<usize> {};

struct U8Expression : private PrimitiveExpression<u8> {};

struct F32Expression : private PrimitiveExpression<f32> {};

struct F64Expression : private PrimitiveExpression<f64> {};

struct BoolExpression {};

struct VoidExpression {};

struct ScopeResolutionExpression {
    NodeID outer;
    NodeID inner;
};

struct StructExpression {
    Members members;
};

struct ExplicitType {
    struct ArrayType {
        opt::Option<NodeID> dimension;
        bool                null_terminated;
        NodeID              inner_type;
    };

    TypeModifier                    modifier;
    std::variant<ArrayType, NodeID> type;
};

struct TypeExpression {
    opt::Option<ExplicitTypeID> explicit_type;
};

struct UnionExpression {
    struct Field {
        NodeID         ident;
        ExplicitTypeID explicit_type;
    };

    std::vector<Field> fields;
    Members            members;
};

struct WhileLoopExpression {
    NodeID              condition;
    opt::Option<NodeID> continuation;
    NodeID              block;
    opt::Option<NodeID> non_break;
};

struct BlockStatement {
    std::vector<NodeID> statements;
};

struct BreakStatement {
    opt::Option<NodeID> label;
    opt::Option<NodeID> expression;
};

struct ContinueStatement {
    opt::Option<NodeID> label;
};

enum class DeclModifiers : u8 {
    VARIABLE  = 1 << 0,
    CONSTANT  = 1 << 1,
    CONSTEXPR = 1 << 2,
    PUBLIC    = 1 << 3,
    EXTERN    = 1 << 4,
    EXPORT    = 1 << 5,
    STATIC    = 1 << 6,
};

constexpr auto operator|(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr auto operator&(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr auto operator^(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

constexpr auto operator|=(DeclModifiers& lhs, DeclModifiers rhs) -> DeclModifiers& {
    lhs = lhs | rhs;
    return lhs;
}

struct DeclStatement {
    NodeID              ident;
    NodeID              type;
    opt::Option<NodeID> value;
    DeclModifiers       modifiers;
};

struct DeferStatement {
    NodeID deferred;
};

struct DiscardStatement {
    NodeID discarded;
};

struct ExpressionStatement {
    NodeID expression;
};

struct ImportStatement {
    NodeID              name;
    opt::Option<NodeID> alias;
};

struct ReturnStatement {
    opt::Option<NodeID> expression;
};

struct TestStatement {
    opt::Option<NodeID> description;
    NodeID              block;
};

struct UsingStatement {
    NodeID         alias;
    ExplicitTypeID explicit_type;
};

} // namespace ast

} // namespace porpoise

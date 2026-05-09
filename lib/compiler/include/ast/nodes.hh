#pragma once

#include <charconv>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

#include "ast/id.hh"

#include "syntax/error.hh"
#include "syntax/parser.hh"

#include "utility.hh"
#include "variant.hh"

namespace porpoise {

namespace ast {

struct DeclStatement;
using DeclHandle = Handle<NodeKind::DECL_STATEMENT>;

struct Members {
    using Member =
        Handle<NodeKind::DECL_STATEMENT, NodeKind::IMPORT_STATEMENT, NodeKind::USING_STATEMENT>;
    MAKE_ITERATOR(MemberList, std::vector<Member>, list)

    MemberList list;

    template <typename MemberValidator>
    [[nodiscard]] static auto parse(syntax::Parser& parser, MemberValidator&& validator)
        -> Result<Members, syntax::Diagnostic> {
        MemberList members;
        while (!parser.peek_token_is(syntax::TokenType::RBRACE) &&
               !parser.peek_token_is(syntax::TokenType::END)) {
            parser.advance();
            auto parsed_member = TRY(parser.parse_statement(true));

            // Downcast the parsed member into the specific member variant and check
            const auto member = TRY(deconstruct_member(parser, parsed_member));
            if (!std::forward<MemberValidator>(validator)(member)) {
                return make_syntax_err(syntax::Error::INVALID_MEMBER,
                                       parser.get_location_of(*member));
            }
            members.emplace_back(member);
        }
        return Members{members};
    }

    static auto validate_struct_decl(const DeclStatement& decl) noexcept -> bool;
    static auto validate_non_struct_decl(const DeclStatement& decl) noexcept -> bool;

  private:
    [[nodiscard]] static auto deconstruct_member(syntax::Parser& parser, StatementHandle member)
        -> Result<Member, syntax::Diagnostic> {
        switch (member->get_kind()) {
        case NodeKind::DECL_STATEMENT:
        case NodeKind::USING_STATEMENT:
        case NodeKind::IMPORT_STATEMENT: return Member{member};
        default:
            return make_syntax_err(syntax::Error::INVALID_MEMBER, parser.get_location_of(*member));
        }
    }
};

struct ArrayExpression {
    opt::Option<ExpressionHandle> size;
    bool                          null_terminated;
    ExplicitTypeID                item_explicit_type;
    std::vector<ExpressionHandle> items;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct CallExpression {
    using Argument = std::variant<ExpressionHandle, ExplicitTypeID>;

    ExpressionHandle      function;
    std::vector<Argument> arguments;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle function)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

using BlockHandle = Handle<NodeKind::BLOCK_STATEMENT>;

struct DoWhileLoopExpression {
    BlockHandle      block;
    ExpressionHandle condition;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

using IdentifierHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION>;

struct EnumExpression {
    using Enumeration = std::pair<IdentifierHandle, opt::Option<ExpressionHandle>>;

    opt::Option<IdentifierHandle> underlying;
    std::vector<Enumeration>      enumerations;
    Members                       members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct ForLoopExpression {
    struct Capture {
        using PayloadHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION, NodeKind::DISCARDED>;

        TypeModifier  modifier;
        PayloadHandle payload;
    };

    std::vector<ExpressionHandle> iterables;
    std::vector<Capture>          captures;
    BlockHandle                   block;
    opt::Option<StatementHandle>  non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct SelfParameter {
    TypeModifier     modifier;
    IdentifierHandle ident;
};

} // namespace ast

namespace traits {

template <> struct Nullable<ast::SelfParameter> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::SelfParameter {
        return ast::SelfParameter{{}, ast::IdentifierHandle::make_invalid()};
    }

    [[nodiscard]] static constexpr auto is_valid(const ast::SelfParameter& self) noexcept -> bool {
        return self.ident.is_valid();
    }
};

} // namespace traits

namespace ast {

struct FunctionExpression {
    struct Parameter {
        opt::Option<IdentifierHandle> ident;
        ExplicitTypeID                explicit_type;
    };

    opt::Option<SelfParameter> self;
    std::vector<Parameter>     parameters;
    bool                       variadic;
    ExplicitTypeID             explicit_return_type;
    opt::Option<BlockHandle>   body;

    // Parse the function as a value. Meant for the parser LUT
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;

    // Parse the function as if it is a type instead of a declaration
    //
    // Meant to be called by the explicit type parser only
    [[nodiscard]] static auto parse_type(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct GroupedExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct IdentifierExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;

    std::string_view name;
};

struct IfExpression {
    bool                         constexpr_condition;
    ExpressionHandle             condition;
    StatementHandle              consequence;
    opt::Option<StatementHandle> alternate;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct IndexExpression {
    ExpressionHandle array;
    ExpressionHandle index;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle array)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct InfiniteLoopExpression {
    BlockHandle block;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

#define DECLARE_INFIX_EXPRESSION(Type)                                                \
    struct Type {                                                                     \
        ExpressionHandle          lhs;                                                \
        ExpressionHandle          rhs;                                                \
        [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle lhs) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;                          \
    };

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(AssignmentExpression)

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(BinaryExpression)

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(DotExpression)

// The operator is stored in the nodes id
DECLARE_INFIX_EXPRESSION(RangeExpression)

#undef DECLARE_INFIX_EXPRESSION

using ImplicitAccessHandle = Handle<NodeKind::IMPLICIT_ACCESS_EXPRESSION>;

struct InitializerExpression {
    struct Initializer {
        ImplicitAccessHandle member;
        ExpressionHandle     value;
    };

    opt::Option<ExpressionHandle> object_type;
    std::vector<Initializer>      initializers;

    // Parse assuming an object is present. Meant for the parser LUT
    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle object)
        -> Result<ExpressionHandle, syntax::Diagnostic> {
        return parse(parser, opt::Option<ExpressionHandle>{object});
    }

    // Parse the expression with a potentially empty object
    [[nodiscard]] static auto parse(syntax::Parser& parser, opt::Option<ExpressionHandle> object)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

using LabeledNodeHandle = Handle<NodeKind::DO_WHILE_LOOP_EXPRESSION,
                                 NodeKind::FOR_LOOP_EXPRESSION,
                                 NodeKind::IF_EXPRESSION,
                                 NodeKind::INFINITE_LOOP_EXPRESSION,
                                 NodeKind::MATCH_EXPRESSION,
                                 NodeKind::WHILE_LOOP_EXPRESSION,
                                 NodeKind::BLOCK_STATEMENT>;

struct LabelExpression {
    IdentifierHandle  name;
    LabeledNodeHandle body;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle name)
        -> Result<ExpressionHandle, syntax::Diagnostic>;

  private:
    [[nodiscard]] static auto deconstruct_body(syntax::Parser& parser, StatementHandle raw_stmt)
        -> Result<LabeledNodeHandle, syntax::Diagnostic>;
};

struct MatchExpression {
    struct Arm {
        using CaptureHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION, NodeKind::DISCARDED>;

        ExpressionHandle           pattern;
        opt::Option<CaptureHandle> capture;
        StatementHandle            dispatch;
    };

    ExpressionHandle             matcher;
    std::vector<Arm>             arms;
    opt::Option<StatementHandle> catch_all;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

#define DECLARE_PREFIX_EXPRESSION(Type)                         \
    struct Type {                                               \
        ExpressionHandle          rhs;                          \
        [[nodiscard]] static auto parse(syntax::Parser& parser) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;    \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression)
DECLARE_PREFIX_EXPRESSION(ImplicitAccessExpression)

#undef DECLARE_PREFIX_EXPRESSION

namespace detail {

// Parses the requested value from the string, asserting the from_chars result if requested
template <typename ValueType, bool AssertLast = true>
[[nodiscard]] static auto parse_primitive_value(std::string_view  slice,
                                                syntax::TokenType type) noexcept
    -> opt::Option<ValueType> {
    const auto  base  = syntax::token_type::to_base(type);
    const auto* first = slice.cbegin() + (!base || *base == syntax::Base::DECIMAL ? 0 : 2);
    const auto* last  = slice.cend() - syntax::token_type::suffix_length(type);

    ValueType              v;
    std::from_chars_result result;
    if constexpr (std::is_same_v<ValueType, f64> || std::is_same_v<ValueType, f32>) {
        result = std::from_chars(first, last, v);
    } else {
        result = std::from_chars(first, last, v, std::to_underlying(*base));
    }

    if constexpr (AssertLast) {
        ASSERT(result.ptr == last);
        if (result.ec == std::errc{}) { return v; }
    } else {
        if (result.ec == std::errc{} && result.ptr == last) { return v; }
    }

    ASSERT(result.ec == std::errc::result_out_of_range);
    return opt::none;
}

} // namespace detail

#define DECLARE_PRIMITIVE_EXPRESSION(NodeType, ValueType)       \
    struct NodeType {                                           \
        using value_type = ValueType;                           \
        value_type                value;                        \
        [[nodiscard]] static auto parse(syntax::Parser& parser) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;    \
    };

DECLARE_PRIMITIVE_EXPRESSION(StringExpression, std::string)
DECLARE_PRIMITIVE_EXPRESSION(I32Expression, i32)
DECLARE_PRIMITIVE_EXPRESSION(I64Expression, i64)
DECLARE_PRIMITIVE_EXPRESSION(ISizeExpression, isize)
DECLARE_PRIMITIVE_EXPRESSION(U32Expression, u32)
DECLARE_PRIMITIVE_EXPRESSION(U64Expression, u64)
DECLARE_PRIMITIVE_EXPRESSION(USizeExpression, usize)
DECLARE_PRIMITIVE_EXPRESSION(U8Expression, u8)
DECLARE_PRIMITIVE_EXPRESSION(F32Expression, f32)
DECLARE_PRIMITIVE_EXPRESSION(F64Expression, f64)

#undef DECLARE_PRIMITIVE_EXPRESSION

namespace traits {

template <typename T> struct is_valued_primitive : std::false_type {};
template <> struct is_valued_primitive<StringExpression> : std::true_type {};
template <> struct is_valued_primitive<I32Expression> : std::true_type {};
template <> struct is_valued_primitive<I64Expression> : std::true_type {};
template <> struct is_valued_primitive<ISizeExpression> : std::true_type {};
template <> struct is_valued_primitive<U32Expression> : std::true_type {};
template <> struct is_valued_primitive<U64Expression> : std::true_type {};
template <> struct is_valued_primitive<USizeExpression> : std::true_type {};
template <> struct is_valued_primitive<U8Expression> : std::true_type {};
template <> struct is_valued_primitive<F32Expression> : std::true_type {};
template <> struct is_valued_primitive<F64Expression> : std::true_type {};
template <typename T> constexpr auto is_primitive_v = is_valued_primitive<T>::value;

// A primitive node with its value embedded in the data
template <typename T>
concept ValuedPrimitiveNode = is_primitive_v<T>;

} // namespace traits

struct BoolExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct VoidExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

namespace traits {

template <typename T> struct is_light_primitive : std::false_type {};
template <> struct is_light_primitive<BoolExpression> : std::true_type {};
template <> struct is_light_primitive<VoidExpression> : std::true_type {};
template <typename T> constexpr auto is_light_primitive_v = is_light_primitive<T>::value;

// A primitive node with its value embedded in its id
template <typename T>
concept LightPrimitiveNode = is_light_primitive_v<T>;

} // namespace traits

struct ScopeResolutionExpression {
    ExpressionHandle outer;
    IdentifierHandle inner;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle outer)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct StructExpression {
    Members members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct ExplicitArrayType {
    opt::Option<ExpressionHandle> dimension;
    bool                          null_terminated;
    ExplicitTypeID                inner_explicit_type;
};

struct UnionExpression {
    struct Field {
        IdentifierHandle ident;
        ExplicitTypeID   explicit_type;
    };

    std::vector<Field> fields;
    Members            members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

using TypeData = std::variant<IdentifierExpression,
                              ScopeResolutionExpression,
                              CallExpression,
                              FunctionExpression,
                              ExplicitTypeID,
                              StructExpression,
                              EnumExpression,
                              UnionExpression,
                              ExplicitArrayType>;

#define FOREACH_AST_TYPE(X)      \
    X(IdentifierExpression)      \
    X(ScopeResolutionExpression) \
    X(CallExpression)            \
    X(FunctionExpression)        \
    X(ExplicitTypeID)            \
    X(StructExpression)          \
    X(EnumExpression)            \
    X(UnionExpression)           \
    X(ExplicitArrayType)

#define AST_TYPE_VISITOR_DEF_GEN_X(NodeType) \
    auto visit(const porpoise::ast::ExplicitTypeID&, const porpoise::ast::NodeType&) -> void;
#define AST_TYPE_VISITOR_DEF_GEN() FOREACH_AST_TYPE(AST_TYPE_VISITOR_DEF_GEN_X)

#define AST_TYPE_VISITOR_NOOP(Class, NodeType)                                              \
    auto Class::visit(const porpoise::ast::ExplicitTypeID&, const porpoise::ast::NodeType&) \
        -> void {}

struct ExplicitType {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExplicitTypeID, syntax::Diagnostic>;
};

namespace traits {

#define ET_KIND_OF_TRAIT(Type, Kind)                                               \
    template <> struct ExplicitTypeKindOf<Type> {                                  \
        [[nodiscard]] static constexpr auto value() noexcept -> ExplicitTypeKind { \
            return ExplicitTypeKind::Kind;                                         \
        }                                                                          \
    };

ET_KIND_OF_TRAIT(IdentifierExpression, IDENT)
ET_KIND_OF_TRAIT(ScopeResolutionExpression, SCOPE)
ET_KIND_OF_TRAIT(CallExpression, CALL)
ET_KIND_OF_TRAIT(FunctionExpression, FUNCTION)
ET_KIND_OF_TRAIT(ExplicitTypeID, RECURSIVE)
ET_KIND_OF_TRAIT(StructExpression, STRUCT)
ET_KIND_OF_TRAIT(EnumExpression, ENUM)
ET_KIND_OF_TRAIT(UnionExpression, UNION)
ET_KIND_OF_TRAIT(ExplicitArrayType, ARRAY)

#undef KIND_OF_TRAIT

} // namespace traits

using TypeHandle = Handle<NodeKind::TYPE_EXPRESSION>;

struct TypeExpression {
    opt::Option<ExplicitTypeID> explicit_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<std::pair<TypeHandle, bool>, syntax::Diagnostic>;
};

struct WhileLoopExpression {
    ExpressionHandle              condition;
    opt::Option<ExpressionHandle> continuation;
    BlockHandle                   block;
    opt::Option<StatementHandle>  non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct BlockStatement {
    MAKE_ITERATOR(Statements, std::vector<StatementHandle>, statements);

    Statements statements;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct BreakStatement {
    opt::Option<IdentifierHandle> label;
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ContinueStatement {
    opt::Option<IdentifierHandle> label;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
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
    IdentifierHandle              ident;
    TypeHandle                    type;
    opt::Option<ExpressionHandle> value;
    DeclModifiers                 modifiers;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;

    [[nodiscard]] auto has_modifier(DeclModifiers flag) const noexcept -> bool {
        return modifiers_has(modifiers, flag);
    }

  private:
    static auto modifiers_has(DeclModifiers modifiers, DeclModifiers flag) noexcept -> bool {
        return static_cast<bool>(modifiers & flag);
    }
};

struct DeferStatement {
    StatementHandle deferred;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct DiscardStatement {
    ExpressionHandle discarded;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ExpressionStatement {
    ExpressionHandle expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser, bool require_semicolon)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

using ImportHandle = ast::Handle<ast::NodeKind::IMPORT_STATEMENT>;

struct ImportStatement {
    using Payload = Handle<NodeKind::STRING_EXPRESSION, NodeKind::IDENTIFIER_EXPRESSION>;

    Payload                       payload;
    opt::Option<IdentifierHandle> alias;

    [[nodiscard]] static constexpr auto is_public(const NodeID& id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct ReturnStatement {
    opt::Option<ExpressionHandle> expression;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

using StringHandle = Handle<NodeKind::STRING_EXPRESSION>;

struct TestStatement {
    opt::Option<StringHandle> description;
    BlockHandle               block;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

struct UsingStatement {
    IdentifierHandle alias;
    ExplicitTypeID   explicit_type;

    [[nodiscard]] static constexpr auto is_public(const NodeID& id) noexcept -> bool {
        return id.get_token_type() == syntax::TokenType::PUBLIC;
    }

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

#define FOREACH_AST_EXPR(X)      \
    X(ArrayExpression)           \
    X(CallExpression)            \
    X(DoWhileLoopExpression)     \
    X(EnumExpression)            \
    X(ForLoopExpression)         \
    X(FunctionExpression)        \
    X(IdentifierExpression)      \
    X(IfExpression)              \
    X(IndexExpression)           \
    X(InfiniteLoopExpression)    \
    X(AssignmentExpression)      \
    X(BinaryExpression)          \
    X(DotExpression)             \
    X(RangeExpression)           \
    X(InitializerExpression)     \
    X(LabelExpression)           \
    X(MatchExpression)           \
    X(ReferenceExpression)       \
    X(DereferenceExpression)     \
    X(UnaryExpression)           \
    X(ImplicitAccessExpression)  \
    X(StringExpression)          \
    X(I32Expression)             \
    X(I64Expression)             \
    X(ISizeExpression)           \
    X(U32Expression)             \
    X(U64Expression)             \
    X(USizeExpression)           \
    X(U8Expression)              \
    X(F32Expression)             \
    X(F64Expression)             \
    X(BoolExpression)            \
    X(VoidExpression)            \
    X(ScopeResolutionExpression) \
    X(StructExpression)          \
    X(TypeExpression)            \
    X(UnionExpression)           \
    X(WhileLoopExpression)

#define FOREACH_AST_STMT(X) \
    X(BlockStatement)       \
    X(BreakStatement)       \
    X(ContinueStatement)    \
    X(DeclStatement)        \
    X(DeferStatement)       \
    X(DiscardStatement)     \
    X(ExpressionStatement)  \
    X(ImportStatement)      \
    X(ReturnStatement)      \
    X(TestStatement)        \
    X(UsingStatement)

#define FOREACH_AST_NODE(X) \
    FOREACH_AST_EXPR(X)     \
    FOREACH_AST_STMT(X)

#define X(Type) Type,
using NodeData = std::variant<FOREACH_AST_NODE(X) Unit>;
#undef X

#define AST_NODE_VISITOR_DEF_GEN_X(NodeType) \
    auto visit(const porpoise::ast::NodeID&, const porpoise::ast::NodeType&) -> void;
#define AST_NODE_VISITOR_DEF_GEN()               \
    FOREACH_AST_NODE(AST_NODE_VISITOR_DEF_GEN_X) \
    auto visit(const porpoise::ast::NodeID&, const porpoise::Unit&) -> void;

#define AST_NODE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(const porpoise::ast::NodeID&, const porpoise::ast::NodeType&) -> void {}

#define AST_VISITOR_DEF_GEN()  \
    AST_NODE_VISITOR_DEF_GEN() \
    AST_TYPE_VISITOR_DEF_GEN()

namespace traits {

#define NODE_KIND_OF_TRAIT(Type, Kind)                                     \
    template <> struct NodeKindOf<Type> {                                  \
        [[nodiscard]] static constexpr auto value() noexcept -> NodeKind { \
            return NodeKind::Kind;                                         \
        }                                                                  \
    };

NODE_KIND_OF_TRAIT(ArrayExpression, ARRAY_EXPRESSION)
NODE_KIND_OF_TRAIT(CallExpression, CALL_EXPRESSION)
NODE_KIND_OF_TRAIT(DoWhileLoopExpression, DO_WHILE_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(EnumExpression, ENUM_EXPRESSION)
NODE_KIND_OF_TRAIT(ForLoopExpression, FOR_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(FunctionExpression, FUNCTION_EXPRESSION)
NODE_KIND_OF_TRAIT(IdentifierExpression, IDENTIFIER_EXPRESSION)
NODE_KIND_OF_TRAIT(IfExpression, IF_EXPRESSION)
NODE_KIND_OF_TRAIT(IndexExpression, INDEX_EXPRESSION)
NODE_KIND_OF_TRAIT(InfiniteLoopExpression, INFINITE_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(AssignmentExpression, ASSIGNMENT_EXPRESSION)
NODE_KIND_OF_TRAIT(BinaryExpression, BINARY_EXPRESSION)
NODE_KIND_OF_TRAIT(DotExpression, DOT_EXPRESSION)
NODE_KIND_OF_TRAIT(RangeExpression, RANGE_EXPRESSION)
NODE_KIND_OF_TRAIT(InitializerExpression, INITIALIZER_EXPRESSION)
NODE_KIND_OF_TRAIT(LabelExpression, LABEL_EXPRESSION)
NODE_KIND_OF_TRAIT(MatchExpression, MATCH_EXPRESSION)
NODE_KIND_OF_TRAIT(UnaryExpression, UNARY_EXPRESSION)
NODE_KIND_OF_TRAIT(ReferenceExpression, REFERENCE_EXPRESSION)
NODE_KIND_OF_TRAIT(DereferenceExpression, DEREFERENCE_EXPRESSION)
NODE_KIND_OF_TRAIT(ImplicitAccessExpression, IMPLICIT_ACCESS_EXPRESSION)
NODE_KIND_OF_TRAIT(StringExpression, STRING_EXPRESSION)
NODE_KIND_OF_TRAIT(I32Expression, I32_EXPRESSION)
NODE_KIND_OF_TRAIT(I64Expression, I64_EXPRESSION)
NODE_KIND_OF_TRAIT(ISizeExpression, ISIZE_EXPRESSION)
NODE_KIND_OF_TRAIT(U32Expression, U32_EXPRESSION)
NODE_KIND_OF_TRAIT(U64Expression, U64_EXPRESSION)
NODE_KIND_OF_TRAIT(USizeExpression, USIZE_EXPRESSION)
NODE_KIND_OF_TRAIT(U8Expression, U8_EXPRESSION)
NODE_KIND_OF_TRAIT(F32Expression, F32_EXPRESSION)
NODE_KIND_OF_TRAIT(F64Expression, F64_EXPRESSION)
NODE_KIND_OF_TRAIT(BoolExpression, BOOL_EXPRESSION)
NODE_KIND_OF_TRAIT(VoidExpression, VOID_EXPRESSION)
NODE_KIND_OF_TRAIT(ScopeResolutionExpression, SCOPE_RESOLUTION_EXPRESSION)
NODE_KIND_OF_TRAIT(StructExpression, STRUCT_EXPRESSION)
NODE_KIND_OF_TRAIT(TypeExpression, TYPE_EXPRESSION)
NODE_KIND_OF_TRAIT(UnionExpression, UNION_EXPRESSION)
NODE_KIND_OF_TRAIT(WhileLoopExpression, WHILE_LOOP_EXPRESSION)

NODE_KIND_OF_TRAIT(BlockStatement, BLOCK_STATEMENT)
NODE_KIND_OF_TRAIT(BreakStatement, BREAK_STATEMENT)
NODE_KIND_OF_TRAIT(ContinueStatement, CONTINUE_STATEMENT)
NODE_KIND_OF_TRAIT(DeclStatement, DECL_STATEMENT)
NODE_KIND_OF_TRAIT(DeferStatement, DEFER_STATEMENT)
NODE_KIND_OF_TRAIT(DiscardStatement, DISCARD_STATEMENT)
NODE_KIND_OF_TRAIT(ExpressionStatement, EXPRESSION_STATEMENT)
NODE_KIND_OF_TRAIT(ImportStatement, IMPORT_STATEMENT)
NODE_KIND_OF_TRAIT(ReturnStatement, RETURN_STATEMENT)
NODE_KIND_OF_TRAIT(TestStatement, TEST_STATEMENT)
NODE_KIND_OF_TRAIT(UsingStatement, USING_STATEMENT)

NODE_KIND_OF_TRAIT(Unit, DISCARDED)

#undef KIND_OF_TRAIT

} // namespace traits

} // namespace ast

} // namespace porpoise

template <> struct magic_enum::customize::enum_range<porpoise::ast::DeclModifiers> {
    static constexpr bool is_flags = true;
};

template <> struct fmt::formatter<porpoise::ast::IdentifierExpression> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::IdentifierExpression& n, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", n.name);
    }
};

template <porpoise::ast::traits::ValuedPrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.value);
    }
};

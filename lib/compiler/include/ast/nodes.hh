#pragma once

#include <vector>

#include "ast/id.hh"

#include "syntax/error.hh"
#include "syntax/parser.hh"

#include "utility.hh"
#include "variant.hh"

namespace porpoise {

namespace ast {

struct Discarded {};

using DeclHandle = Handle<NodeKind::DECL_STATEMENT>;

struct Members {
    using Member =
        Handle<NodeKind::DECL_STATEMENT, NodeKind::IMPORT_STATEMENT, NodeKind::USING_STATEMENT>;

    std::vector<Member> list;

    template <typename MemberValidator>
    [[nodiscard]] static auto parse(syntax::Parser& parser, MemberValidator&& validator)
        -> Result<Members, syntax::Diagnostic>;

    static auto validate_struct_decl(const DeclHandle& decl) noexcept -> bool;
    static auto validate_non_struct_decl(const DeclHandle& decl) noexcept -> bool;

  private:
    [[nodiscard]] static auto deconstruct_member(StatementHandle member)
        -> Result<Member, syntax::Diagnostic>;
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
    std::vector<Enumeration>      enumeration;
    Members                       members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct ForLoopExpression {
    struct Capture {
        using PayloadHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION, NodeKind::DISCARDED>;

        opt::Option<TypeModifier> modifier;
        PayloadHandle             payload;
    };

    std::vector<ExpressionHandle> iterables;
    std::vector<Capture>          captures;
    BlockHandle                   block;
    opt::Option<StatementHandle>  non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct FunctionExpression {
    struct Self {
        opt::Option<TypeModifier> modifier;
        IdentifierHandle          ident;
    };

    struct Parameter {
        opt::Option<IdentifierHandle> ident;
        ExplicitTypeID                explicit_type;
    };

    Self                     self;
    std::vector<Parameter>   parameters;
    bool                     variadic;
    ExplicitTypeID           explicit_return_type;
    opt::Option<BlockHandle> body;

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
};

struct IfExpression {
    bool                         constexpr_condition;
    ExpressionHandle             condition;
    ExpressionHandle             consequence;
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
        syntax::TokenType         op;                                                 \
        ExpressionHandle          rhs;                                                \
        [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle lhs) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;                          \
    };

DECLARE_INFIX_EXPRESSION(AssignmentExpression)
DECLARE_INFIX_EXPRESSION(BinaryExpression)
DECLARE_INFIX_EXPRESSION(DotExpression)
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
        -> Result<ExpressionHandle, syntax::Diagnostic>;

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
    [[nodiscard]] static auto deconstruct_body(StatementHandle raw_stmt)
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
        NodeID                    rhs;                          \
        [[nodiscard]] static auto parse(syntax::Parser& parser) \
            -> Result<ExpressionHandle, syntax::Diagnostic>;    \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression)
DECLARE_PREFIX_EXPRESSION(ImplicitAccessExpression)

#undef DECLARE_PREFIX_EXPRESSION

#define DECLARE_PRIMITIVE_EXPRESSION(NodeType, ValueType)       \
    struct NodeType {                                           \
        ValueType                 value;                        \
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

struct BoolExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct VoidExpression {
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct ScopeResolutionExpression {
    ExpressionHandle     outer;
    IdentifierExpression inner;

    [[nodiscard]] static auto parse(syntax::Parser& parser, ExpressionHandle outer)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct StructExpression {
    Members members;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct ExplicitType {
    struct ArrayType {
        opt::Option<NodeID> dimension;
        bool                null_terminated;
        ExplicitTypeID      inner_explicit_type;
    };

    using TypedExpressionHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION,
                                         NodeKind::SCOPE_RESOLUTION_EXPRESSION,
                                         NodeKind::CALL_EXPRESSION,
                                         NodeKind::FUNCTION_EXPRESSION,
                                         NodeKind::STRUCT_EXPRESSION,
                                         NodeKind::ENUM_EXPRESSION,
                                         NodeKind::UNION_EXPRESSION>;

    TypeModifier                                                   modifier;
    std::variant<ArrayType, TypedExpressionHandle, ExplicitTypeID> type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExplicitTypeID, syntax::Diagnostic>;
};

struct TypeExpression {
    opt::Option<ExplicitTypeID> explicit_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
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

struct WhileLoopExpression {
    ExpressionHandle              condition;
    opt::Option<ExpressionHandle> continuation;
    BlockHandle                   block;
    opt::Option<StatementHandle>  non_break;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<ExpressionHandle, syntax::Diagnostic>;
};

struct BlockStatement {
    std::vector<StatementHandle> statements;

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

using TypeHandle = Handle<NodeKind::TYPE_EXPRESSION>;

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

struct ImportStatement {
    using ImportedExpressionHandle =
        Handle<NodeKind::STRING_EXPRESSION, NodeKind::IDENTIFIER_EXPRESSION>;

    ImportedExpressionHandle      name;
    opt::Option<ExpressionHandle> alias;

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
    IdentifierExpression alias;
    ExplicitTypeID       explicit_type;

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<StatementHandle, syntax::Diagnostic>;
};

using NodeData = std::variant<ArrayExpression,
                              CallExpression,
                              DoWhileLoopExpression,
                              EnumExpression,
                              ForLoopExpression,
                              FunctionExpression,
                              IdentifierExpression,
                              IfExpression,
                              IndexExpression,
                              InfiniteLoopExpression,
                              AssignmentExpression,
                              BinaryExpression,
                              DotExpression,
                              RangeExpression,
                              InitializerExpression,
                              LabelExpression,
                              MatchExpression,
                              UnaryExpression,
                              ReferenceExpression,
                              DereferenceExpression,
                              ImplicitAccessExpression,
                              StringExpression,
                              I32Expression,
                              I64Expression,
                              ISizeExpression,
                              U32Expression,
                              U64Expression,
                              USizeExpression,
                              U8Expression,
                              F32Expression,
                              F64Expression,
                              BoolExpression,
                              VoidExpression,
                              ScopeResolutionExpression,
                              StructExpression,
                              TypeExpression,
                              UnionExpression,
                              WhileLoopExpression,
                              BlockStatement,
                              BreakStatement,
                              ContinueStatement,
                              DeclStatement,
                              DeferStatement,
                              DiscardStatement,
                              ExpressionStatement,
                              ImportStatement,
                              ReturnStatement,
                              TestStatement,
                              UsingStatement>;

namespace traits {

#define KIND_OF_TRAIT(Type, Kind)                                          \
    template <> struct NodeKindOf<Type> {                                  \
        [[nodiscard]] static constexpr auto value() noexcept -> NodeKind { \
            return NodeKind::Kind;                                         \
        }                                                                  \
    };

KIND_OF_TRAIT(Discarded, DISCARDED)

KIND_OF_TRAIT(ArrayExpression, ARRAY_EXPRESSION)
KIND_OF_TRAIT(CallExpression, CALL_EXPRESSION)
KIND_OF_TRAIT(DoWhileLoopExpression, DO_WHILE_LOOP_EXPRESSION)
KIND_OF_TRAIT(EnumExpression, ENUM_EXPRESSION)
KIND_OF_TRAIT(ForLoopExpression, FOR_LOOP_EXPRESSION)
KIND_OF_TRAIT(FunctionExpression, FUNCTION_EXPRESSION)
KIND_OF_TRAIT(IdentifierExpression, IDENTIFIER_EXPRESSION)
KIND_OF_TRAIT(IfExpression, IF_EXPRESSION)
KIND_OF_TRAIT(IndexExpression, INDEX_EXPRESSION)
KIND_OF_TRAIT(InfiniteLoopExpression, INFINITE_LOOP_EXPRESSION)
KIND_OF_TRAIT(AssignmentExpression, ASSIGNMENT_EXPRESSION)
KIND_OF_TRAIT(BinaryExpression, BINARY_EXPRESSION)
KIND_OF_TRAIT(DotExpression, DOT_EXPRESSION)
KIND_OF_TRAIT(RangeExpression, RANGE_EXPRESSION)
KIND_OF_TRAIT(InitializerExpression, INITIALIZER_EXPRESSION)
KIND_OF_TRAIT(LabelExpression, LABEL_EXPRESSION)
KIND_OF_TRAIT(MatchExpression, MATCH_EXPRESSION)
KIND_OF_TRAIT(UnaryExpression, UNARY_EXPRESSION)
KIND_OF_TRAIT(ReferenceExpression, REFERENCE_EXPRESSION)
KIND_OF_TRAIT(DereferenceExpression, DEREFERENCE_EXPRESSION)
KIND_OF_TRAIT(ImplicitAccessExpression, IMPLICIT_ACCESS_EXPRESSION)
KIND_OF_TRAIT(StringExpression, STRING_EXPRESSION)
KIND_OF_TRAIT(I32Expression, I32_EXPRESSION)
KIND_OF_TRAIT(I64Expression, I64_EXPRESSION)
KIND_OF_TRAIT(ISizeExpression, ISIZE_EXPRESSION)
KIND_OF_TRAIT(U32Expression, U32_EXPRESSION)
KIND_OF_TRAIT(U64Expression, U64_EXPRESSION)
KIND_OF_TRAIT(USizeExpression, USIZE_EXPRESSION)
KIND_OF_TRAIT(U8Expression, U8_EXPRESSION)
KIND_OF_TRAIT(F32Expression, F32_EXPRESSION)
KIND_OF_TRAIT(F64Expression, F64_EXPRESSION)
KIND_OF_TRAIT(BoolExpression, BOOL_EXPRESSION)
KIND_OF_TRAIT(VoidExpression, VOID_EXPRESSION)
KIND_OF_TRAIT(ScopeResolutionExpression, SCOPE_RESOLUTION_EXPRESSION)
KIND_OF_TRAIT(StructExpression, STRUCT_EXPRESSION)
KIND_OF_TRAIT(TypeExpression, TYPE_EXPRESSION)
KIND_OF_TRAIT(UnionExpression, UNION_EXPRESSION)
KIND_OF_TRAIT(WhileLoopExpression, WHILE_LOOP_EXPRESSION)

KIND_OF_TRAIT(BlockStatement, BLOCK_STATEMENT)
KIND_OF_TRAIT(BreakStatement, BREAK_STATEMENT)
KIND_OF_TRAIT(ContinueStatement, CONTINUE_STATEMENT)
KIND_OF_TRAIT(DeclStatement, DECL_STATEMENT)
KIND_OF_TRAIT(DeferStatement, DEFER_STATEMENT)
KIND_OF_TRAIT(DiscardStatement, DISCARD_STATEMENT)
KIND_OF_TRAIT(ExpressionStatement, EXPRESSION_STATEMENT)
KIND_OF_TRAIT(ImportStatement, IMPORT_STATEMENT)
KIND_OF_TRAIT(ReturnStatement, RETURN_STATEMENT)
KIND_OF_TRAIT(TestStatement, TEST_STATEMENT)
KIND_OF_TRAIT(UsingStatement, USING_STATEMENT)

#undef KIND_OF_TRAIT

} // namespace traits

} // namespace ast

} // namespace porpoise

template <> struct magic_enum::customize::enum_range<porpoise::ast::DeclModifiers> {
    static constexpr bool is_flags = true;
};

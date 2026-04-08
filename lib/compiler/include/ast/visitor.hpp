#pragma once

namespace porpoise::ast {

#define FOREACH_AST_EXPR(X)      \
    X(ArrayExpression)           \
    X(CallArgument)              \
    X(CallExpression)            \
    X(DoWhileLoopExpression)     \
    X(Enumeration)               \
    X(EnumExpression)            \
    X(ForLoopCapture)            \
    X(ForLoopExpression)         \
    X(SelfParameter)             \
    X(FunctionParameter)         \
    X(FunctionExpression)        \
    X(IdentifierExpression)      \
    X(IfExpression)              \
    X(IndexExpression)           \
    X(InfiniteLoopExpression)    \
    X(AssignmentExpression)      \
    X(BinaryExpression)          \
    X(DotExpression)             \
    X(RangeExpression)           \
    X(Initializer)               \
    X(InitializerExpression)     \
    X(MatchArm)                  \
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
    X(ScopeResolutionExpression) \
    X(StructExpression)          \
    X(ExplicitType)              \
    X(TypeExpression)            \
    X(UnionField)                \
    X(UnionExpression)           \
    X(WhileLoopExpression)

#define FOREACH_AST_STMT(X) \
    X(BlockStatement)       \
    X(DeclStatement)        \
    X(DeferStatement)       \
    X(DiscardStatement)     \
    X(ExpressionStatement)  \
    X(ImportStatement)      \
    X(JumpStatement)        \
    X(ModuleStatement)      \
    X(UsingStatement)

#define FOREACH_AST_NODE(X) \
    FOREACH_AST_EXPR(X)     \
    FOREACH_AST_STMT(X)

// The following use '()' since it messes with Zed's syntax highlighting otherwise :(

#define FORWARD_DECLARE_NODE(NodeType) class NodeType;
#define FORWARD_DECLARE_AST_NODES() FOREACH_AST_NODE(FORWARD_DECLARE_NODE)

#define GENERATE_ABSTRACT_AST_VISITOR(NodeType) \
    virtual auto visit(const porpoise::ast::NodeType&) -> void = 0;
#define ABSTRACT_AST_VISITOR_DECLARATION() FOREACH_AST_NODE(GENERATE_ABSTRACT_AST_VISITOR)

#define GENERATE_OVERRIDE_AST_VISITOR(NodeType) \
    auto visit(const porpoise::ast::NodeType&) -> void override;
#define MAKE_AST_VISITOR_OVERRIDES() FOREACH_AST_NODE(GENERATE_OVERRIDE_AST_VISITOR)

#define GENERATE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(const ast::NodeType&) -> void {}

FORWARD_DECLARE_AST_NODES()

class Visitor {
  public:
    virtual ~Visitor() = default;
    ABSTRACT_AST_VISITOR_DECLARATION()
};

} // namespace porpoise::ast

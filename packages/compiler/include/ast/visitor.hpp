#pragma once

namespace conch::ast {

#define FOREACH_AST_NODE(X)          \
    X(ArrayExpression)               \
    X(AssignmentExpression)          \
    X(BinaryExpression)              \
    X(CallExpression)                \
    X(DoWhileLoopExpression)         \
    X(DotExpression)                 \
    X(EnumExpression)                \
    X(ForLoopExpression)             \
    X(FunctionExpression)            \
    X(IdentifierExpression)          \
    X(IfExpression)                  \
    X(IndexExpression)               \
    X(InfiniteLoopExpression)        \
    X(MatchExpression)               \
    X(ReferenceExpression)           \
    X(DereferenceExpression)         \
    X(UnaryExpression)               \
    X(StringExpression)              \
    X(SignedIntegerExpression)       \
    X(SignedLongIntegerExpression)   \
    X(ISizeIntegerExpression)        \
    X(UnsignedIntegerExpression)     \
    X(UnsignedLongIntegerExpression) \
    X(USizeIntegerExpression)        \
    X(ByteExpression)                \
    X(FloatExpression)               \
    X(BoolExpression)                \
    X(RangeExpression)               \
    X(ScopeResolutionExpression)     \
    X(StructExpression)              \
    X(TypeExpression)                \
    X(WhileLoopExpression)           \
    X(BlockStatement)                \
    X(DeclStatement)                 \
    X(DiscardStatement)              \
    X(ExpressionStatement)           \
    X(ImportStatement)               \
    X(JumpStatement)

#define FORWARD_DECLARE_NODE(NodeType) class NodeType;
#define FORWARD_DECLARE_AST_NODES FOREACH_AST_NODE(FORWARD_DECLARE_NODE)

#define GENERATE_ABSTRACT_AST_VISITOR(NodeType) virtual auto visit(const NodeType&) -> void = 0;
#define ABSTRACT_AST_VISITOR_DECLARATION FOREACH_AST_NODE(GENERATE_ABSTRACT_AST_VISITOR)

#define GENERATE_OVERRIDE_AST_VISITOR(NodeType) auto visit(const NodeType&) -> void override;
#define AST_VISITOR_OVERRIDES FOREACH_AST_NODE(GENERATE_OVERRIDE_AST_VISITOR)

FORWARD_DECLARE_AST_NODES

class Visitor {
  public:
    virtual ~Visitor() = default;
    ABSTRACT_AST_VISITOR_DECLARATION
};

} // namespace conch::ast

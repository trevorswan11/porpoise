#include "sema/type_resolver.hpp"

#include "sema/analyzer.hpp"

namespace porpoise::sema {

TypeResolver::TypeResolver(Analyzer& analyzer, SymbolTableStack& stack) noexcept
    : analyzer_{analyzer}, pool_{analyzer_.get_pool()}, stack_{stack} {}

auto TypeResolver::visit(const ast::ArrayExpression&) -> void {}

auto TypeResolver::visit(const ast::CallArgument&) -> void {}

auto TypeResolver::visit(const ast::CallExpression&) -> void {}

auto TypeResolver::visit(const ast::DoWhileLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::Enumeration&) -> void {}

auto TypeResolver::visit(const ast::EnumExpression&) -> void {}

auto TypeResolver::visit(const ast::ForLoopCapture&) -> void {}

auto TypeResolver::visit(const ast::ForLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::SelfParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionParameter&) -> void {}

auto TypeResolver::visit(const ast::FunctionExpression&) -> void {}

auto TypeResolver::visit(const ast::IdentifierExpression&) -> void {}

auto TypeResolver::visit(const ast::IfExpression&) -> void {}

auto TypeResolver::visit(const ast::IndexExpression&) -> void {}

auto TypeResolver::visit(const ast::InfiniteLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::AssignmentExpression&) -> void {}

auto TypeResolver::visit(const ast::BinaryExpression&) -> void {}

auto TypeResolver::visit(const ast::DotExpression&) -> void {}

auto TypeResolver::visit(const ast::RangeExpression&) -> void {}

auto TypeResolver::visit(const ast::Initializer&) -> void {}

auto TypeResolver::visit(const ast::InitializerExpression&) -> void {}

auto TypeResolver::visit(const ast::MatchArm&) -> void {}

auto TypeResolver::visit(const ast::MatchExpression&) -> void {}

auto TypeResolver::visit(const ast::ReferenceExpression&) -> void {}

auto TypeResolver::visit(const ast::DereferenceExpression&) -> void {}

auto TypeResolver::visit(const ast::UnaryExpression&) -> void {}

auto TypeResolver::visit(const ast::ImplicitAccessExpression&) -> void {}

auto TypeResolver::visit(const ast::StringExpression&) -> void {}

#define MAKE_BUILTIN_RESOLVER(NodeType, kind)                \
    auto TypeResolver::visit(const ast::NodeType&) -> void { \
        last_type_.emplace(pool_.builtin(TypeKind::kind));   \
    }

MAKE_BUILTIN_RESOLVER(I32Expression, INT)
MAKE_BUILTIN_RESOLVER(I64Expression, LONG)
MAKE_BUILTIN_RESOLVER(ISizeExpression, SIZE)
MAKE_BUILTIN_RESOLVER(U32Expression, UINT)
MAKE_BUILTIN_RESOLVER(U64Expression, ULONG)
MAKE_BUILTIN_RESOLVER(USizeExpression, USIZE)
MAKE_BUILTIN_RESOLVER(U8Expression, BYTE)
MAKE_BUILTIN_RESOLVER(BoolExpression, BOOL)
MAKE_BUILTIN_RESOLVER(VoidExpression, VOID)
MAKE_BUILTIN_RESOLVER(F32Expression, FLOAT)
MAKE_BUILTIN_RESOLVER(F64Expression, DOUBLE)

auto TypeResolver::visit(const ast::ScopeResolutionExpression&) -> void {}

auto TypeResolver::visit(const ast::StructExpression&) -> void {}

auto TypeResolver::visit(const ast::ExplicitType&) -> void {}

auto TypeResolver::visit(const ast::TypeExpression&) -> void {}

auto TypeResolver::visit(const ast::UnionField&) -> void {}

auto TypeResolver::visit(const ast::UnionExpression&) -> void {}

auto TypeResolver::visit(const ast::WhileLoopExpression&) -> void {}

auto TypeResolver::visit(const ast::BlockStatement&) -> void {}

auto TypeResolver::visit(const ast::DeclStatement&) -> void {}

auto TypeResolver::visit(const ast::DeferStatement&) -> void {}

auto TypeResolver::visit(const ast::DiscardStatement&) -> void {}

auto TypeResolver::visit(const ast::ExpressionStatement&) -> void {}

auto TypeResolver::visit(const ast::ImportStatement&) -> void {}

auto TypeResolver::visit(const ast::JumpStatement&) -> void {}

auto TypeResolver::visit(const ast::ModuleStatement&) -> void {}

auto TypeResolver::visit(const ast::TestStatement&) -> void {}

auto TypeResolver::visit(const ast::UsingStatement&) -> void {}

} // namespace porpoise::sema

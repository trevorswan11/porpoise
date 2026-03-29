#include "sema/type_resolver.hpp"

namespace porpoise::sema {

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
auto TypeResolver::visit(const ast::MatchArm&) -> void {}
auto TypeResolver::visit(const ast::MatchExpression&) -> void {}
auto TypeResolver::visit(const ast::ReferenceExpression&) -> void {}
auto TypeResolver::visit(const ast::DereferenceExpression&) -> void {}
auto TypeResolver::visit(const ast::ImplicitAccessExpression&) -> void {}
auto TypeResolver::visit(const ast::UnaryExpression&) -> void {}
auto TypeResolver::visit(const ast::StringExpression&) -> void {}
auto TypeResolver::visit(const ast::SignedIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::SignedLongIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::ISizeIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::UnsignedIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::UnsignedLongIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::USizeIntegerExpression&) -> void {}
auto TypeResolver::visit(const ast::ByteExpression&) -> void {}
auto TypeResolver::visit(const ast::FloatExpression&) -> void {}
auto TypeResolver::visit(const ast::DoubleExpression&) -> void {}
auto TypeResolver::visit(const ast::BoolExpression&) -> void {}
auto TypeResolver::visit(const ast::ScopeResolutionExpression&) -> void {}
auto TypeResolver::visit(const ast::StructExpression&) -> void {}
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
auto TypeResolver::visit(const ast::UsingStatement&) -> void {}

} // namespace porpoise::sema

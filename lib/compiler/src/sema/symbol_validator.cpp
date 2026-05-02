#include "sema/symbol_validator.hpp"

namespace porpoise::sema {

auto SymbolValidator::visit(const ast::ArrayExpression&) -> void {}

auto SymbolValidator::visit(const ast::CallArgument&) -> void {}

auto SymbolValidator::visit(const ast::CallExpression&) -> void {}

auto SymbolValidator::visit(const ast::DoWhileLoopExpression&) -> void {}

auto SymbolValidator::visit(const ast::Enumeration&) -> void {}

auto SymbolValidator::visit(const ast::EnumExpression&) -> void {}

auto SymbolValidator::visit(const ast::ForLoopCapture&) -> void {}

auto SymbolValidator::visit(const ast::ForLoopExpression&) -> void {}

auto SymbolValidator::visit(const ast::SelfParameter&) -> void {}

auto SymbolValidator::visit(const ast::FunctionParameter&) -> void {}

auto SymbolValidator::visit(const ast::FunctionExpression&) -> void {}

auto SymbolValidator::visit(const ast::IdentifierExpression&) -> void {}

auto SymbolValidator::visit(const ast::IfExpression&) -> void {}

auto SymbolValidator::visit(const ast::IndexExpression&) -> void {}

auto SymbolValidator::visit(const ast::InfiniteLoopExpression&) -> void {}

auto SymbolValidator::visit(const ast::AssignmentExpression&) -> void {}

auto SymbolValidator::visit(const ast::BinaryExpression&) -> void {}

auto SymbolValidator::visit(const ast::DotExpression&) -> void {}

auto SymbolValidator::visit(const ast::RangeExpression&) -> void {}

auto SymbolValidator::visit(const ast::Initializer&) -> void {}

auto SymbolValidator::visit(const ast::InitializerExpression&) -> void {}

auto SymbolValidator::visit(const ast::LabelExpression&) -> void {}

auto SymbolValidator::visit(const ast::MatchArm&) -> void {}

auto SymbolValidator::visit(const ast::MatchExpression&) -> void {}

auto SymbolValidator::visit(const ast::ReferenceExpression&) -> void {}

auto SymbolValidator::visit(const ast::DereferenceExpression&) -> void {}

auto SymbolValidator::visit(const ast::UnaryExpression&) -> void {}

auto SymbolValidator::visit(const ast::ImplicitAccessExpression&) -> void {}

auto SymbolValidator::visit(const ast::StringExpression&) -> void {}

auto SymbolValidator::visit(const ast::I32Expression&) -> void {}

auto SymbolValidator::visit(const ast::I64Expression&) -> void {}

auto SymbolValidator::visit(const ast::ISizeExpression&) -> void {}

auto SymbolValidator::visit(const ast::U32Expression&) -> void {}

auto SymbolValidator::visit(const ast::U64Expression&) -> void {}

auto SymbolValidator::visit(const ast::USizeExpression&) -> void {}

auto SymbolValidator::visit(const ast::U8Expression&) -> void {}

auto SymbolValidator::visit(const ast::BoolExpression&) -> void {}

auto SymbolValidator::visit(const ast::VoidExpression&) -> void {}

auto SymbolValidator::visit(const ast::F32Expression&) -> void {}

auto SymbolValidator::visit(const ast::F64Expression&) -> void {}

auto SymbolValidator::visit(const ast::ScopeResolutionExpression&) -> void {}

auto SymbolValidator::visit(const ast::StructExpression&) -> void {}

auto SymbolValidator::visit(const ast::ExplicitType&) -> void {}

auto SymbolValidator::visit(const ast::TypeExpression&) -> void {}

auto SymbolValidator::visit(const ast::UnionField&) -> void {}

auto SymbolValidator::visit(const ast::UnionExpression&) -> void {}

auto SymbolValidator::visit(const ast::WhileLoopExpression&) -> void {}

auto SymbolValidator::visit(const ast::BlockStatement&) -> void {}

auto SymbolValidator::visit(const ast::BreakStatement&) -> void {}

auto SymbolValidator::visit(const ast::ContinueStatement&) -> void {}

auto SymbolValidator::visit(const ast::DeclStatement&) -> void {}

auto SymbolValidator::visit(const ast::DeferStatement&) -> void {}

auto SymbolValidator::visit(const ast::DiscardStatement&) -> void {}

auto SymbolValidator::visit(const ast::ExpressionStatement&) -> void {}

auto SymbolValidator::visit(const ast::ImportStatement&) -> void {}

auto SymbolValidator::visit(const ast::ReturnStatement&) -> void {}

auto SymbolValidator::visit(const ast::TestStatement&) -> void {}

auto SymbolValidator::visit(const ast::UsingStatement&) -> void {}

} // namespace porpoise::sema

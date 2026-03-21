#pragma once

#include <utility>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;
class USizeIntegerExpression;

class ExplicitType;

class ExplicitArrayType {
  public:
    explicit ExplicitArrayType(Optional<Box<Expression>> dimension,
                               bool                      null_terminated,
                               Box<ExplicitType>         inner_type) noexcept;
    ~ExplicitArrayType();

    MAKE_AST_COPY_MOVE(ExplicitArrayType)

    MAKE_OPTIONAL_UNPACKER(dimension, Expression, dimension_, **)
    [[nodiscard]] auto is_null_terminated() const noexcept -> bool { return null_terminated_; }
    MAKE_GETTER(inner_type, const ExplicitType&, *)

    MAKE_EQ_DELEGATION(ExplicitArrayType)

  private:
    Optional<Box<Expression>> dimension_;
    bool                      null_terminated_;
    Box<ExplicitType>         inner_type_;
};

class ExplicitType {
  public:
    using ExplicitIdentType     = Box<IdentifierExpression>;
    using ExplicitFunctionType  = Box<FunctionExpression>;
    using ExplicitRecursiveType = Box<ExplicitType>;

    using ExplicitTypeVariant = std::
        variant<ExplicitIdentType, ExplicitFunctionType, ExplicitArrayType, ExplicitRecursiveType>;

  public:
    explicit ExplicitType(TypeModifier modifier, ExplicitTypeVariant type) noexcept;
    ~ExplicitType();

    MAKE_AST_COPY_MOVE(ExplicitType)

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<ExplicitType, syntax::ParserDiagnostic>;

    MAKE_GETTER(modifier, const TypeModifier&)
    MAKE_GETTER(type, const ExplicitTypeVariant&)
    MAKE_VARIANT_UNPACKER(ident_type, IdentifierExpression, ExplicitIdentType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(function_type, FunctionExpression, ExplicitFunctionType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(array_type, ExplicitArrayType, ExplicitArrayType, type_, std::get)
    MAKE_VARIANT_UNPACKER(
        recursive_type, ExplicitRecursiveType, ExplicitRecursiveType, type_, std::get)

    MAKE_EQ_DELEGATION(ExplicitType)

  private:
    TypeModifier        modifier_;
    ExplicitTypeVariant type_;
};

class TypeExpression : public ExprBase<TypeExpression> {
  public:
    static constexpr auto KIND = NodeKind::TYPE_EXPRESSION;

  public:
    explicit TypeExpression(const syntax::Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    MAKE_AST_COPY_MOVE(TypeExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<std::pair<Box<Expression>, bool>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(explicit_type, ExplicitType, explicit_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<ExplicitType> explicit_;

    // FunctionExpression needs to destructively move explicit on success
    friend class FunctionExpression;
};

} // namespace porpoise::ast

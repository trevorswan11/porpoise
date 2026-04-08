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
class USizeExpression;
class StructExpression;
class EnumExpression;
class UnionExpression;

class ExplicitType;

class ExplicitArrayType {
  public:
    ExplicitArrayType(Optional<mem::Box<Expression>> dimension,
                      bool                           null_terminated,
                      mem::Box<ExplicitType>         inner_type) noexcept;
    ~ExplicitArrayType();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ExplicitArrayType)

    MAKE_OPTIONAL_UNPACKER(dimension, Expression, dimension_, **)
    [[nodiscard]] auto is_null_terminated() const noexcept -> bool { return null_terminated_; }
    MAKE_GETTER(inner_type, const ExplicitType&, *)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(ExplicitArrayType)

  private:
    Optional<mem::Box<Expression>> dimension_;
    bool                           null_terminated_;
    mem::Box<ExplicitType>         inner_type_;
};

class ExplicitType {
  public:
    using ExplicitIdentType     = mem::Box<IdentifierExpression>;
    using ExplicitFunctionType  = mem::Box<FunctionExpression>;
    using ExplicitRecursiveType = mem::Box<ExplicitType>;
    using ExplicitStructType    = mem::Box<StructExpression>;
    using ExplicitEnumType      = mem::Box<EnumExpression>;
    using ExplicitUnionType     = mem::Box<UnionExpression>;

    using ExplicitTypeVariant = std::variant<ExplicitIdentType,
                                             ExplicitFunctionType,
                                             ExplicitArrayType,
                                             ExplicitRecursiveType,
                                             ExplicitStructType,
                                             ExplicitEnumType,
                                             ExplicitUnionType>;

  public:
    ExplicitType(TypeModifier modifier, ExplicitTypeVariant type) noexcept;
    ~ExplicitType();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ExplicitType)

    auto                      accept(Visitor& v) const -> void;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<ExplicitType, syntax::ParserDiagnostic>;

    MAKE_GETTER(modifier, const TypeModifier&)
    MAKE_GETTER(type, const ExplicitTypeVariant&)
    MAKE_VARIANT_UNPACKER(ident_type, IdentifierExpression, ExplicitIdentType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(function_type, FunctionExpression, ExplicitFunctionType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(array_type, ExplicitArrayType, ExplicitArrayType, type_, std::get)
    MAKE_VARIANT_UNPACKER(
        recursive_type, ExplicitRecursiveType, ExplicitRecursiveType, type_, std::get)
    MAKE_VARIANT_UNPACKER(struct_type, StructExpression, ExplicitStructType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(enum_type, EnumExpression, ExplicitEnumType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(union_type, UnionExpression, ExplicitUnionType, type_, *std::get)
    MAKE_VARIANT_MATCHER(type_)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(ExplicitType)

  private:
    TypeModifier        modifier_;
    ExplicitTypeVariant type_;
};

class TypeExpression : public ExprBase<TypeExpression> {
  public:
    static constexpr auto KIND = NodeKind::TYPE_EXPRESSION;

  public:
    TypeExpression(const syntax::Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(TypeExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<std::pair<mem::Box<Expression>, bool>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(explicit_type, ExplicitType, explicit_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<ExplicitType> explicit_;

    // Function's need to destructively move explicit on success
    friend class FunctionExpression;
    friend class FunctionParameter;
};

} // namespace porpoise::ast

#pragma once

#include <span>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;
class ExplicitType;
class BlockStatement;

class SelfParameter {
  public:
    SelfParameter(TypeModifier modifier, mem::Box<IdentifierExpression> name) noexcept;
    ~SelfParameter();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(SelfParameter)

    auto accept(Visitor& v) const -> void;

    MAKE_GETTER(modifier, const TypeModifier&)
    MAKE_GETTER(ident, const IdentifierExpression&, *)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(SelfParameter)

  private:
    TypeModifier                   modifier_;
    mem::Box<IdentifierExpression> ident_;
};

class FunctionParameter {
  public:
    FunctionParameter(mem::Box<IdentifierExpression> ident, ExplicitType&& type) noexcept;
    explicit FunctionParameter(ExplicitType&& type) noexcept;
    ~FunctionParameter();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(FunctionParameter)

    auto accept(Visitor& v) const -> void;

    MAKE_NULLABLE_BOX_UNPACKER(ident, IdentifierExpression, ident_, *)
    MAKE_GETTER(type, const ExplicitType&)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(FunctionParameter)

  private:
    mem::NullableBox<IdentifierExpression> ident_;
    ExplicitType                           type_;
};

class FunctionExpression : public ExprBase<FunctionExpression> {
  public:
    static constexpr auto KIND = NodeKind::FUNCTION_EXPRESSION;

  public:
    FunctionExpression(const syntax::Token&             start_token,
                       Optional<SelfParameter>          self,
                       std::vector<FunctionParameter>   parameters,
                       bool                             variadic,
                       ExplicitType&&                   return_type,
                       mem::NullableBox<BlockStatement> body) noexcept;
    ~FunctionExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(FunctionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    // Meant to be called by the explicit type parser only
    [[nodiscard]] static auto parse_type(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(self, SelfParameter, self_, *)
    MAKE_GETTER(parameters, std::span<const FunctionParameter>)
    [[nodiscard]] auto has_parameters() const noexcept -> bool { return !parameters_.empty(); }
    [[nodiscard]] auto is_variadic() const noexcept -> bool { return variadic_; }
    MAKE_GETTER(return_type, const ExplicitType&)
    MAKE_NULLABLE_BOX_UNPACKER(body, BlockStatement, body_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<SelfParameter>          self_;
    std::vector<FunctionParameter>   parameters_;
    bool                             variadic_;
    ExplicitType                     return_type_;
    mem::NullableBox<BlockStatement> body_;
};

} // namespace porpoise::ast

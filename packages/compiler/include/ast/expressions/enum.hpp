#pragma once

#include <span>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class Enumeration {
  public:
    explicit Enumeration(Box<IdentifierExpression> ident, Optional<Box<Expression>> value) noexcept;
    ~Enumeration();

    MAKE_AST_COPY_MOVE(Enumeration)

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_OPTIONAL_UNPACKER(default_value, Expression, value_, **)

    MAKE_EQ_DELEGATION(Enumeration)

  private:
    Box<IdentifierExpression> ident_;
    Optional<Box<Expression>> value_;
};

class EnumExpression : public ExprBase<EnumExpression> {
  public:
    static constexpr auto KIND = NodeKind::ENUM_EXPRESSION;

  public:
    explicit EnumExpression(const syntax::Token&                start_token,
                            Optional<Box<IdentifierExpression>> underlying,
                            std::vector<Enumeration>            enumerations) noexcept;
    ~EnumExpression() override;

    MAKE_AST_COPY_MOVE(EnumExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(underlying, IdentifierExpression, underlying_, **)
    MAKE_GETTER(enumerations, std::span<const Enumeration>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<Box<IdentifierExpression>> underlying_;
    std::vector<Enumeration>            enumerations_;
};

} // namespace porpoise::ast

#pragma once

#include <span>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class Enumeration {
  public:
    Enumeration(mem::Box<IdentifierExpression> ident,
                Optional<mem::Box<Expression>> value) noexcept;
    ~Enumeration();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(Enumeration)

    auto accept(Visitor& v) const -> void;

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_OPTIONAL_UNPACKER(default_value, Expression, value_, **)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(Enumeration)

  private:
    mem::Box<IdentifierExpression> ident_;
    Optional<mem::Box<Expression>> value_;
};

class EnumExpression : public ExprBase<EnumExpression> {
  public:
    static constexpr auto KIND = NodeKind::ENUM_EXPRESSION;

  public:
    MAKE_ITERATOR(Enumerations, std::vector<Enumeration>, enumerations_)

  public:
    EnumExpression(const syntax::Token&                     start_token,
                   Optional<mem::Box<IdentifierExpression>> underlying,
                   Enumerations                             enumerations) noexcept;
    ~EnumExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(EnumExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(underlying, IdentifierExpression, underlying_, **)
    MAKE_GETTER(enumerations, std::span<const Enumeration>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<mem::Box<IdentifierExpression>> underlying_;
    Enumerations                             enumerations_;
};

} // namespace porpoise::ast

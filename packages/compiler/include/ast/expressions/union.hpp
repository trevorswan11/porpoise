#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class UnionField {
  public:
    explicit UnionField(mem::Box<IdentifierExpression> ident, ExplicitType&& type) noexcept;
    ~UnionField();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(UnionField)

    MAKE_GETTER(ident, const IdentifierExpression&, *)
    MAKE_GETTER(type, const ExplicitType&)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token&;

    MAKE_EQ_DELEGATION(UnionField)

  private:
    mem::Box<IdentifierExpression> ident_;
    ExplicitType                   type_;
};

class UnionExpression : public ExprBase<UnionExpression> {
  public:
    static constexpr auto KIND = NodeKind::UNION_EXPRESSION;

  public:
    MAKE_ITERATOR(Fields, std::vector<UnionField>, fields_)

  public:
    explicit UnionExpression(const syntax::Token& start_token, Fields fields) noexcept;
    ~UnionExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(UnionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(fields, std::span<const UnionField>)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Fields fields_;
};

} // namespace porpoise::ast

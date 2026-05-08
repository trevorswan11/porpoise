#pragma once

#include "ast/oop_node.hh"
#include "ast/statements/members.hh"

#include "syntax/parser.hh"

namespace porpoise::ast {

class FunctionExpression;

class StructExpression : public ExprBase<StructExpression> {
  public:
    static constexpr auto KIND = NodeKind::STRUCT_EXPRESSION;

  public:
    StructExpression(const syntax::Token& start_token, Members members) noexcept;
    ~StructExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(StructExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::Diagnostic>;

    MAKE_GETTER(members, const Members&)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Members members_;
};

} // namespace porpoise::ast

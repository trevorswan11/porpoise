#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "iterator.hpp"

namespace porpoise::ast {

class FunctionExpression;

class StructExpression : public ExprBase<StructExpression> {
  public:
    static constexpr auto KIND = NodeKind::STRUCT_EXPRESSION;

  public:
    MAKE_UNALIASED_ITERATOR(Members, members_)

  public:
    StructExpression(const syntax::Token& start_token, Members members) noexcept;
    ~StructExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(StructExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(members, MembersView)
    [[nodiscard]] auto is_packed() const noexcept -> bool {
        return start_token_.type == syntax::TokenType::PACKED;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Members members_;
};

} // namespace porpoise::ast

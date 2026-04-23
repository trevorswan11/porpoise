#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {
    
class IdentifierExpression;
    
class LabelExpression : public ExprBase<LabelExpression> {
  public:
    static constexpr auto KIND = NodeKind::LABEL_EXPRESSION;

  public:
    LabelExpression(const syntax::Token& start_token,
                    mem::Box<IdentifierExpression> name) noexcept;
    ~LabelExpression();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(LabelExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser, mem::Box<Expression> name)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(name, const IdentifierExpression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<IdentifierExpression> name_;
};

}
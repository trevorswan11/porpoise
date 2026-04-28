#pragma once

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class UsingStatement : public StmtBase<UsingStatement> {
  public:
    static constexpr auto KIND = NodeKind::USING_STATEMENT;

  public:
    UsingStatement(const syntax::Token&           start_token,
                   mem::Box<IdentifierExpression> alias,
                   ExplicitType&&                 type) noexcept;
    ~UsingStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(UsingStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_GETTER(alias, const IdentifierExpression&, *)
    MAKE_GETTER(type, const ExplicitType&)

    [[nodiscard]] auto is_public() const noexcept -> bool {
        return start_token_.type == syntax::TokenType::PUBLIC;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<IdentifierExpression> alias_;
    ExplicitType                   type_;
};

} // namespace porpoise::ast

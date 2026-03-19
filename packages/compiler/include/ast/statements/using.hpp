#pragma once

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class UsingStatement : public StmtBase<UsingStatement> {
  public:
    static constexpr auto KIND = NodeKind::USING_STATEMENT;

  public:
    explicit UsingStatement(const Token&              start_token,
                            Box<IdentifierExpression> alias,
                            ExplicitType&&            type) noexcept;
    ~UsingStatement() override;

    MAKE_AST_COPY_MOVE(UsingStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    MAKE_GETTER(alias, const IdentifierExpression&, *)
    MAKE_GETTER(type, const ExplicitType&)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<IdentifierExpression> alias_;
    ExplicitType              type_;
};

} // namespace porpoise::ast

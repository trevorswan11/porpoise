#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class ContinueStatement : public StmtBase<ContinueStatement> {
  public:
    static constexpr auto KIND = NodeKind::CONTINUE_STATEMENT;

  public:
    ContinueStatement(const syntax::Token&                   start_token,
                      mem::NullableBox<IdentifierExpression> label) noexcept;
    ~ContinueStatement() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ContinueStatement)

    auto                      accept(Visitor& v) const noexcept -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(label, IdentifierExpression, label_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::NullableBox<IdentifierExpression> label_;
};

} // namespace porpoise::ast

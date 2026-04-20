#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class JumpStatement : public StmtBase<JumpStatement> {
  public:
    static constexpr auto KIND = NodeKind::JUMP_STATEMENT;

  public:
    JumpStatement(const syntax::Token&         start_token,
                  mem::NullableBox<Expression> expression) noexcept
        : StmtBase{start_token}, expression_{std::move(expression)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(JumpStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(expression, Expression, expression_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<JumpStatement>(other);
        return mem::nullable_boxes_eq(expression_, casted.expression_);
    }

  private:
    mem::NullableBox<Expression> expression_;
};

} // namespace porpoise::ast

#pragma once

#include <utility>

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression;

template <typename Derived> class JumpStmtBase : public StmtBase<Derived> {
  public:
    JumpStmtBase(const syntax::Token& start_token, mem::NullableBox<Expression> expression) noexcept
        : StmtBase<Derived>{start_token}, expression_{std::move(expression)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(JumpStmtBase)

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    MAKE_NULLABLE_BOX_UNPACKER(expression, Expression, expression_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return mem::nullable_boxes_eq(expression_, casted.expression_);
    }

  protected:
    mem::NullableBox<Expression> expression_;
};

class JumpStatement : public JumpStmtBase<JumpStatement> {
  public:
    static constexpr auto KIND = NodeKind::JUMP_STATEMENT;

  public:
    JumpStatement(const syntax::Token&                   start_token,
                  mem::NullableBox<IdentifierExpression> label,
                  mem::NullableBox<Expression>           expression) noexcept;
    ~JumpStatement();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(JumpStatement)

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_NULLABLE_BOX_UNPACKER(label, IdentifierExpression, label_, *)
    [[nodiscard]] auto is_continue() const noexcept -> bool;
    [[nodiscard]] auto is_break() const noexcept -> bool;

  private:
    mem::NullableBox<IdentifierExpression> label_;
};

class ReturnStatement : public JumpStmtBase<ReturnStatement> {
  public:
    static constexpr auto KIND = NodeKind::RETURN_STATEMENT;

  public:
    using JumpStmtBase::JumpStmtBase;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ReturnStatement)

    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Statement>, syntax::ParserDiagnostic>;
};

} // namespace porpoise::ast

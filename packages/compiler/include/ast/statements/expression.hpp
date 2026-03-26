#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class ExpressionStatement : public StmtBase<ExpressionStatement> {
  public:
    static constexpr auto KIND = NodeKind::EXPRESSION_STATEMENT;

  public:
    explicit ExpressionStatement(const syntax::Token& start_token,
                                 mem::Box<Expression> expression) noexcept
        : StmtBase{start_token}, expression_{std::move(expression)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ExpressionStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_GETTER(expression, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<ExpressionStatement>(other);
        return *expression_ == *casted.expression_;
    }

  private:
    mem::Box<Expression> expression_;
};

} // namespace porpoise::ast

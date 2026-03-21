#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class DeferStatement : public StmtBase<DeferStatement> {
  public:
    static constexpr auto KIND = NodeKind::DEFER_STATEMENT;

  public:
    explicit DeferStatement(const syntax::Token& start_token, Box<Statement> deferred) noexcept
        : StmtBase{start_token}, deferred_{std::move(deferred)} {}

    MAKE_AST_COPY_MOVE(DeferStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_GETTER(deferred, const Statement&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<DeferStatement>(other);
        return *deferred_ == *casted.deferred_;
    }

  private:
    Box<Statement> deferred_;
};

} // namespace porpoise::ast

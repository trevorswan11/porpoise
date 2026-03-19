#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace porpoise::ast {

class ModuleStatement : public StmtBase<ModuleStatement> {
  public:
    static constexpr auto KIND = NodeKind::MODULE_STATEMENT;

  public:
    explicit ModuleStatement(const Token& start_token) noexcept : StmtBase{start_token} {}

    MAKE_AST_COPY_MOVE(ModuleStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

} // namespace porpoise::ast

#pragma once

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class ModuleStatement : public StmtBase<ModuleStatement> {
  public:
    static constexpr auto KIND = NodeKind::MODULE_STATEMENT;

  public:
    explicit ModuleStatement(const syntax::Token& start_token) noexcept : StmtBase{start_token} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(ModuleStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

} // namespace porpoise::ast

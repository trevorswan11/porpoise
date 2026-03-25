#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class BlockStatement : public StmtBase<BlockStatement> {
  public:
    static constexpr auto KIND = NodeKind::BLOCK_STATEMENT;

  public:
    MAKE_ITERATOR(Statements, std::vector<mem::Box<Statement>>, statements_)

  public:
    explicit BlockStatement(const syntax::Token& start_token, Statements statements) noexcept
        : StmtBase{start_token}, statements_{std::move(statements)} {}

    MAKE_AST_COPY_MOVE(BlockStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<BlockStatement>(other);
        return std::ranges::equal(
            statements_, casted.statements_, [](const auto& a, const auto& b) { return *a == *b; });
    }

  private:
    Statements statements_;
};

} // namespace porpoise::ast

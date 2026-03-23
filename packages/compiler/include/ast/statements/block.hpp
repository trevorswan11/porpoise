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
    using iterator       = typename std::vector<mem::Box<Statement>>::iterator;
    using const_iterator = typename std::vector<mem::Box<Statement>>::const_iterator;

  public:
    explicit BlockStatement(const syntax::Token&             start_token,
                            std::vector<mem::Box<Statement>> statements) noexcept
        : StmtBase{start_token}, statements_{std::move(statements)} {}

    MAKE_AST_COPY_MOVE(BlockStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    [[nodiscard]] auto begin() noexcept -> iterator { return statements_.begin(); }
    [[nodiscard]] auto end() noexcept -> iterator { return statements_.end(); }

    [[nodiscard]] auto begin() const noexcept -> const_iterator { return statements_.begin(); }
    [[nodiscard]] auto end() const noexcept -> const_iterator { return statements_.end(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t { return statements_.size(); }
    [[nodiscard]] auto empty() const noexcept -> bool { return statements_.empty(); }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<BlockStatement>(other);
        return std::ranges::equal(
            statements_, casted.statements_, [](const auto& a, const auto& b) { return *a == *b; });
    }

  private:
    std::vector<mem::Box<Statement>> statements_;
};

} // namespace porpoise::ast

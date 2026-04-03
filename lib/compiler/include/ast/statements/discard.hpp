#pragma once

#include <utility>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class DiscardStatement : public StmtBase<DiscardStatement> {
  public:
    static constexpr auto KIND = NodeKind::DISCARD_STATEMENT;

  public:
    DiscardStatement(const syntax::Token& start_token, mem::Box<Expression> discarded) noexcept
        : StmtBase{start_token}, discarded_{std::move(discarded)} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(DiscardStatement)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic>;

    MAKE_GETTER(discarded, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<DiscardStatement>(other);
        return *discarded_ == *casted.discarded_;
    }

  private:
    mem::Box<Expression> discarded_;
};

} // namespace porpoise::ast

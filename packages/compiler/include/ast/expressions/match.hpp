#pragma once

#include <algorithm>
#include <span>
#include <utility>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class MatchArm {
  public:
    explicit MatchArm(Box<Expression> pattern, Box<Statement> dispatch) noexcept
        : pattern_{std::move(pattern)}, dispatch_{std::move(dispatch)} {}

    MAKE_AST_COPY_MOVE(MatchArm)

    MAKE_AST_GETTER(pattern, const Expression&, *)
    MAKE_AST_GETTER(dispatch, const Statement&, *)

    MAKE_AST_DEPENDENT_EQ(MatchArm)

  private:
    Box<Expression> pattern_;
    Box<Statement>  dispatch_;
};

class MatchExpression : public ExprBase<MatchExpression> {
  public:
    static constexpr auto KIND = NodeKind::MATCH_EXPRESSION;

  public:
    explicit MatchExpression(const Token&             start_token,
                             Box<Expression>          matcher,
                             std::vector<MatchArm>    arms,
                             Optional<Box<Statement>> catch_all) noexcept
        : ExprBase{start_token}, matcher_{std::move(matcher)}, arms_{std::move(arms)},
          catch_all_{std::move(catch_all)} {}

    MAKE_AST_COPY_MOVE(MatchExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(matcher, const Expression&, *)
    MAKE_AST_GETTER(arms, std::span<const MatchArm>, )
    MAKE_OPTIONAL_UNPACKER(catch_all, Statement, catch_all_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted  = as<MatchExpression>(other);
        const auto  arms_eq = std::ranges::equal(arms_, casted.arms_);
        return *matcher_ == *casted.matcher_ && arms_eq &&
               optional::unsafe_eq<Statement>(catch_all_, casted.catch_all_);
    }

  private:
    Box<Expression>          matcher_;
    std::vector<MatchArm>    arms_;
    Optional<Box<Statement>> catch_all_;
};

} // namespace conch::ast

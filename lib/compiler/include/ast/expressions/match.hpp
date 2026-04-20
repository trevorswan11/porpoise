#pragma once

#include <span>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

#include "variant.hpp"

namespace porpoise::ast {

class IdentifierExpression;

class MatchArm {
  public:
    using Capture = std::variant<mem::Box<IdentifierExpression>, Unit>;

  public:
    MatchArm(mem::Box<Expression> pattern,
             opt::Option<Capture> capture,
             mem::Box<Statement>  dispatch) noexcept;
    ~MatchArm();

    MAKE_MOVE_CONSTRUCTABLE_ONLY(MatchArm)

    auto accept(Visitor& v) const -> void;

    MAKE_GETTER(pattern, const Expression&, *)
    [[nodiscard]] auto has_capture_clause() const noexcept -> bool { return capture_.has_value(); }
    MAKE_VARIANT_UNPACKER(explicit_capture,
                          IdentifierExpression,
                          mem::Box<IdentifierExpression>,
                          *capture_,
                          *std::get)

    [[nodiscard]] auto is_discarded_capture() const noexcept -> bool {
        return std::holds_alternative<Unit>(*capture_);
    }

    MAKE_GETTER(dispatch, const Statement&, *)
    [[nodiscard]] auto get_token() const noexcept -> const syntax::Token& {
        return pattern_->get_token();
    }

    MAKE_AST_SEMA_TYPE_FNS()

    MAKE_EQ_DELEGATION(MatchArm)

  private:
    mem::Box<Expression>             pattern_;
    opt::Option<Capture>             capture_;
    mem::Box<Statement>              dispatch_;
    mutable opt::Option<sema::Type&> sema_type_;
};

class MatchExpression : public ExprBase<MatchExpression> {
  public:
    static constexpr auto KIND = NodeKind::MATCH_EXPRESSION;

  public:
    MatchExpression(const syntax::Token&        start_token,
                    mem::Box<Expression>        matcher,
                    std::vector<MatchArm>       arms,
                    mem::NullableBox<Statement> catch_all) noexcept;
    ~MatchExpression() override;

    MAKE_MOVE_CONSTRUCTABLE_ONLY(MatchExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    MAKE_GETTER(matcher, const Expression&, *)
    MAKE_GETTER(arms, std::span<const MatchArm>)
    MAKE_NULLABLE_BOX_UNPACKER(catch_all, Statement, catch_all_, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    mem::Box<Expression>        matcher_;
    std::vector<MatchArm>       arms_;
    mem::NullableBox<Statement> catch_all_;
};

} // namespace porpoise::ast

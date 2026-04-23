#pragma once

#include <string>
#include <string_view>

#include <fmt/format.h>

#include "ast/node.hpp"

#include "syntax/parser.hpp"

namespace porpoise::ast {

class IdentifierExpression : public ExprBase<IdentifierExpression> {
  public:
    static constexpr auto KIND = NodeKind::IDENTIFIER_EXPRESSION;

  public:
    explicit IdentifierExpression(const syntax::Token& start_token) noexcept
        : ExprBase{start_token} {}

    MAKE_MOVE_CONSTRUCTABLE_ONLY(IdentifierExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(syntax::Parser& parser)
        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic>;

    [[nodiscard]] auto get_name() const noexcept -> std::string_view { return get_token().slice; }
    [[nodiscard]] auto materialize() const -> std::string { return std::string{get_name()}; }

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

} // namespace porpoise::ast

template <> struct fmt::formatter<porpoise::ast::IdentifierExpression> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const porpoise::ast::IdentifierExpression& n, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", n.get_name());
    }
};

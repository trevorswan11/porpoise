#include "ast/expressions/scope_resolve.hh"

#include "ast/expressions/identifier.hh"
#include "ast/visitor.hh"

namespace porpoise::ast {

ScopeResolutionExpression::ScopeResolutionExpression(const syntax::Token&           start_token,
                                                     mem::Box<Expression>           outer,
                                                     mem::Box<IdentifierExpression> inner) noexcept
    : ExprBase{start_token}, outer_{std::move(outer)}, inner_{std::move(inner)} {}
ScopeResolutionExpression::~ScopeResolutionExpression() = default;

auto ScopeResolutionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ScopeResolutionExpression::parse(syntax::Parser& parser, mem::Box<Expression> outer)
    -> Result<mem::Box<Expression>, syntax::Diagnostic> {
    if (!outer->any<IdentifierExpression, ScopeResolutionExpression>()) {
        return make_syntax_err(syntax::Error::ILLEGAL_OUTER_SCOPE_TYPE, outer->get_token());
    }

    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    auto inner = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    return mem::make_box<ScopeResolutionExpression>(
        outer->get_token(), std::move(outer), std::move(inner));
}

auto ScopeResolutionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ScopeResolutionExpression>(other);
    return *outer_ == *casted.outer_ && *inner_ == *casted.inner_;
}

} // namespace porpoise::ast

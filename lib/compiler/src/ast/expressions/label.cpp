#include "ast/expressions/label.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

LabelExpression::LabelExpression(const syntax::Token&           start_token,
                                 mem::Box<IdentifierExpression> name) noexcept
    : ExprBase{start_token}, name_{std::move(name)} {}
LabelExpression::~LabelExpression() = default;

auto LabelExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto LabelExpression::parse(syntax::Parser& parser, mem::Box<Expression> name)
    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {
    const auto& start_token = parser.get_current_token();
    if (!name->is<IdentifierExpression>()) {
        return make_parser_err(syntax::ParserError::ILLEGAL_LABEL, start_token);
    }

    return mem::make_box<LabelExpression>(start_token,
                                          downcast<IdentifierExpression>(std::move(name)));
}

auto LabelExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<LabelExpression>(other);
    return *name_ == *casted.name_;
}

} // namespace porpoise::ast

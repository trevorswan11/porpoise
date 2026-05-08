#include "ast/expressions/struct.hh"

#include "ast/statements/declaration.hh" // IWYU pragma: keep
#include "ast/visitor.hh"

namespace porpoise::ast {

StructExpression::StructExpression(const syntax::Token& start_token, Members members) noexcept
    : ExprBase{start_token}, members_{std::move(members)} {}
StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(syntax::Parser& parser)
    -> Result<mem::Box<Expression>, syntax::Diagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::LBRACE));
    auto members = TRY(Members::parse(parser,
                                      Overloaded{[](const mem::Box<DeclStatement>& decl) {
                                                     return Members::validate_struct_decl(*decl);
                                                 },
                                                 [](const auto&) { return true; }}));
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return mem::make_box<StructExpression>(start_token, std::move(members));
}

auto StructExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<StructExpression>(other);
    return members_ == casted.members_;
}

} // namespace porpoise::ast

#include <utility>

#include "ast/statements/using.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

UsingStatement::UsingStatement(const syntax::Token&      start_token,
                               Box<IdentifierExpression> alias,
                               ExplicitType&&            type) noexcept
    : StmtBase{start_token}, alias_{std::move(alias)}, type_{std::move(type)} {}
UsingStatement::~UsingStatement() = default;

auto UsingStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto UsingStatement::parse(syntax::Parser& parser)
    -> Expected<Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(syntax::TokenType::IDENT));
    auto alias = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

    TRY(parser.expect_peek(syntax::TokenType::ASSIGN));
    auto type = TRY(ExplicitType::parse(parser));

    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return make_box<UsingStatement>(start_token, std::move(alias), std::move(type));
}

auto UsingStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<UsingStatement>(other);
    return *alias_ == *casted.alias_ && type_ == casted.type_;
}

} // namespace porpoise::ast

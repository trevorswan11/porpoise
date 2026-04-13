#include "ast/statements/module.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

auto ModuleStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ModuleStatement::parse(syntax::Parser& parser)
    -> Expected<mem::Box<Statement>, syntax::ParserDiagnostic> {
    const auto start_token = parser.get_current_token();
    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));
    return mem::make_box<ModuleStatement>(start_token);
}

} // namespace porpoise::ast

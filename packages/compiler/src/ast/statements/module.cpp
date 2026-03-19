#include "ast/statements/module.hpp"
#include "ast/visitor.hpp"

namespace porpoise::ast {

auto ModuleStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ModuleStatement::parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType::SEMICOLON));
    return make_box<ModuleStatement>(start_token);
}

} // namespace porpoise::ast

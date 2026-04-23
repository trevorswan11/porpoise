#include "helpers/sema.hpp"

namespace porpoise::tests::helpers {

auto analyze(std::string_view input) -> std::pair<sema::Analyzer, usize> {
    syntax::Parser p{input};
    auto [ast, parser_errors] = p.consume();
    CHECK_FALSE(ast.empty());
    check_errors<syntax::ParserDiagnostic>(parser_errors);

    sema::Analyzer analyzer{std::move(ast)};
    const auto     idx = analyzer.collect_symbols();
    return {std::move(analyzer), idx};
}

auto analyze_and_validate(std::string_view input) -> std::pair<sema::Analyzer, usize> {
    auto [analyzer, idx] = analyze(input);
    check_errors<sema::Diagnostic>(analyzer.get_diagnostics());
    return {std::move(analyzer), idx};
}

auto common_decl(std::string_view name, std::string_view assign) -> ast::DeclStatement {
    return ast::DeclStatement{
        syntax::Token{syntax::keywords::CONSTANT},
        helpers::make_ident(name),
        mem::make_box<ast::TypeExpression>(syntax::Token{syntax::operators::WALRUS}, opt::none),
        helpers::make_ident<true>(assign),
        ast::DeclModifiers::CONSTANT,
    };
}

} // namespace porpoise::tests::helpers

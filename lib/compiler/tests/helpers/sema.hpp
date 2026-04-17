#pragma once

#include <tuple>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/ast.hpp"
#include "helpers/common.hpp"

#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/symbol.hpp"

#include "ast/statements/declaration.hpp"
#include "ast/statements/import.hpp"
#include "ast/statements/using.hpp"

#include "syntax/parser.hpp"

namespace porpoise::tests::helpers {

// Verifies that the collectors output matches the provided pairs
template <typename... KVs>
auto test_collector(std::string_view input, bool is_module, KVs&&... kvs) -> sema::Analyzer {
    syntax::Parser p{input};
    auto [ast, parser_errors] = p.consume();
    CHECK_FALSE(ast.empty());
    check_errors<syntax::ParserDiagnostic>(parser_errors);

    sema::Analyzer analyzer{std::move(ast)};
    const auto     idx = analyzer.collect_symbols();
    check_errors<sema::Diagnostic>(analyzer.get_diagnostics());
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == sizeof...(KVs));
    CHECK(actual.is_module() == is_module);

    (..., [&] mutable {
        using SymbolicNodeType = decltype(std::get<1>(kvs));
        if constexpr (ast::is_leaf_node_v<std::remove_cvref_t<SymbolicNodeType>>) {
            CHECK(
                std::get<1>(kvs)
                    .template any<ast::DeclStatement, ast::ImportStatement, ast::UsingStatement>());
        }

        const auto opt = actual.get_opt(std::get<0>(kvs));
        CHECK(opt);
        sema::Symbol expected{std::get<0>(kvs), &std::get<1>(kvs)};
        if constexpr (std::tuple_size<std::remove_cvref_t<decltype(kvs)>>{} > 2) {
            const auto type = analyzer.get_pool().get(std::get<2>(kvs));
            CHECK(type);
            expected.emplace_type(*type);
        }

        CHECK(*opt == expected);
    }());
    return analyzer;
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    auto [ast, parser_errors] = p.consume();
    CHECK_FALSE(ast.empty());
    CHECK(parser_errors.empty());

    const std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto       expected_count = sizeof...(Ds);

    sema::Analyzer analyzer{std::move(ast)};
    CHECK(analyzer.get_table_opt(analyzer.collect_symbols()));
    const auto& errors = analyzer.get_diagnostics();

    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        CHECK(errors.size() == expected_count);
    }
    CHECK(std::ranges::equal(errors, expected_arr));
}

// A common decl for tests formatted as `const name := assign;`
template <bool Alloc> auto common_decl(std::string_view name, std::string_view assign) {
    auto decl = ast::DeclStatement{
        syntax::Token{syntax::keywords::CONSTANT},
        helpers::make_ident(name),
        mem::make_box<ast::TypeExpression>(syntax::Token{syntax::operators::WALRUS}, std::nullopt),
        helpers::make_ident<true>(assign),
        ast::DeclModifiers::CONSTANT,
    };

    if constexpr (Alloc) {
        return mem::make_box<ast::DeclStatement>(std::move(decl));
    } else {
        return decl;
    }
}

inline auto foo_bar_decl() -> ast::DeclStatement { return common_decl<false>("foo", "bar"); }

// Shallowly checks the symbols in the inner scope of a statement
template <typename... MakerPair>
auto test_hollow_symbols(sema::Analyzer& analyzer, MakerPair&&... pair) -> void {
    auto& registry = analyzer.get_registry();
    CHECK(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(pair));

    (..., [&] {
        const auto         name          = std::get<0>(pair);
        const auto         expected_decl = std::get<1>(pair)();
        const sema::Symbol expected{name, &expected_decl};
        CHECK(expected == registry.get_from(1, name));
    }());
}

} // namespace porpoise::tests::helpers

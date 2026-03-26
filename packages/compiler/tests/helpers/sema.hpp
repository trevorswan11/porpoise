#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

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

    (
        [&](auto&& kv) mutable {
            CHECK(
                std::get<1>(kv)
                    .template any<ast::DeclStatement, ast::ImportStatement, ast::UsingStatement>());

            const auto opt = actual.get_opt(std::get<0>(kv));
            CHECK(opt);
            const sema::Symbol expected{std::get<0>(kv), &std::get<1>(kv)};
            if constexpr (std::tuple_size<std::remove_cvref_t<decltype(kv)>>{} > 2) {
                const auto type = analyzer.get_pool().get(std::get<2>(kv));
                CHECK(type);
                expected.emplace_type(*type);
            }

            CHECK(*opt == expected);
        }(std::forward<KVs>(kvs)),
        ...);
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

} // namespace porpoise::tests::helpers

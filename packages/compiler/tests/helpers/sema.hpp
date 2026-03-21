#pragma once

#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/common.hpp"

#include "sema/collector.hpp"
#include "sema/error.hpp"
#include "sema/symbol.hpp"

#include "ast/statements/declaration.hpp"
#include "ast/statements/import.hpp"
#include "ast/statements/using.hpp"

#include "syntax/parser.hpp"

namespace porpoise::tests::helpers {

// Verifies that the collectors output matches the provided pairs
template <typename... KVs>
auto test_collector(std::string_view input, bool is_module, KVs&&... kvs) -> void {
    syntax::Parser p{input};
    auto [ast, parser_errors] = p.consume();
    CHECK_FALSE(ast.empty());
    check_errors<syntax::ParserDiagnostic>(parser_errors);

    auto [actual, errors] = sema::SymbolCollector::collect(ast);
    check_errors<sema::SemaDiagnostic>(errors);
    CHECK(actual.size() == sizeof...(KVs));
    CHECK(actual.is_module() == is_module);

    (
        [&](auto&& kv) mutable {
            CHECK(
                kv.second
                    .template any<ast::DeclStatement, ast::ImportStatement, ast::UsingStatement>());

            CHECK(actual.has(kv.first));
            const auto opt = actual.get_opt(kv.first);
            CHECK(opt.has_value());
            const sema::Symbol expected{kv.first, &kv.second};
            CHECK(*opt == expected);
        }(std::forward<KVs>(kvs)),
        ...);
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::SemaDiagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    auto [ast, parser_errors] = p.consume();
    CHECK_FALSE(ast.empty());
    CHECK(parser_errors.empty());

    std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto expected_count = sizeof...(Ds);

    auto [_, errors] = sema::SymbolCollector::collect(ast);
    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        CHECK(errors.size() == expected_count);
    }
    CHECK(std::ranges::equal(errors, expected_arr));
}

} // namespace porpoise::tests::helpers

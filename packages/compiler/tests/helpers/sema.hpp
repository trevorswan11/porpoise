#pragma once

#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/common.hpp"

#include "sema/collector.hpp"
#include "sema/error.hpp"
#include "sema/symbol.hpp"

#include "parser/parser.hpp"

namespace porpoise::tests::helpers {

// Verifies that the collectors output matches the provided pairs
template <typename... KVs>
    requires(std::same_as<KVs, sema::SymbolTable::KV> && ...)
auto test_collector(std::string_view input, KVs&&... kvs) -> void {
    Parser p{input};
    auto [ast, parser_errors] = p.consume();
    REQUIRE_FALSE(ast.empty());
    check_errors<ParserDiagnostic>(parser_errors);

    auto [table, errors] = sema::SymbolCollector::collect(ast);
    check_errors<sema::SemaDiagnostic>(errors);
    REQUIRE(table.size() == sizeof...(KVs));

    // clang-format off
    ([&](auto&& kv) {
        const auto actual = table.get_opt(kv.first);
        REQUIRE(actual.has_value());
        REQUIRE(*actual == kv.second);
    }(std::forward<KVs>(kvs)), ...);
    // clang-format on
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::SemaDiagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    Parser p{failing};
    auto [ast, parser_errors] = p.consume();
    REQUIRE_FALSE(ast.empty());
    REQUIRE(parser_errors.empty());

    std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto expected_count = sizeof...(Ds);

    auto [_, errors] = sema::SymbolCollector::collect(ast);
    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        REQUIRE(errors.size() == expected_count);
    }
    REQUIRE(std::ranges::equal(errors, expected_arr));
}

} // namespace porpoise::tests::helpers

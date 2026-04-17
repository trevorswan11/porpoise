#pragma once

#include <tuple>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/ast.hpp" // IWYU pragma: keep
#include "helpers/common.hpp"

#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/symbol.hpp"

#include "syntax/parser.hpp"

namespace porpoise::tests::helpers {

// Analyzes the assumed-syntactically-valid input and returns the analyzer and parent table index.
[[nodiscard]] auto analyze(std::string_view input) -> std::pair<sema::Analyzer, usize>;

// Verifies that the collectors output matches the provided pairs
template <typename... KVs>
auto test_collector(std::string_view input, bool is_module, KVs&&... kvs) -> sema::Analyzer {
    auto [analyzer, idx] = analyze(input);
    check_errors<sema::Diagnostic>(analyzer.get_diagnostics());
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == sizeof...(KVs));
    CHECK(actual.is_module() == is_module);

    (..., [&] mutable {
        const auto opt = actual.get_opt(std::get<0>(kvs));
        REQUIRE(opt);
        sema::Symbol expected{std::get<0>(kvs), &std::get<1>(kvs)};

        // A third tuple param is allowed to be a type
        if constexpr (std::tuple_size<std::remove_cvref_t<decltype(kvs)>>{} > 2) {
            const auto type = analyzer.get_pool().get(std::get<2>(kvs));
            REQUIRE(type);
            expected.emplace_type(*type);
        }

        CHECK(*opt == expected);
    }());
    return std::move(analyzer);
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    syntax::Parser p{failing};
    auto [ast, parser_errors] = p.consume();
    REQUIRE_FALSE(ast.empty());
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
auto        common_decl(std::string_view name, std::string_view assign) -> ast::DeclStatement;
inline auto foo_bar_decl() -> ast::DeclStatement { return common_decl("foo", "bar"); }

// Shallowly checks the symbols in the inner scope of a statement
template <typename... MakerPair>
auto test_hollow_symbols(sema::Analyzer& analyzer, MakerPair&&... pair) -> void {
    auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(pair));

    (..., [&] {
        const auto         name          = std::get<0>(pair);
        const auto         expected_decl = std::get<1>(pair)();
        const sema::Symbol expected{name, &expected_decl};
        CHECK(expected == registry.get_from(1, name));
    }());
}

} // namespace porpoise::tests::helpers

#pragma once

#include <tuple>

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

template <ast::LeafNode N> struct TableEntry {
    std::string_view              name;
    N                             node;
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

// Verifies that the collectors output matches the provided pairs
template <typename... KVs>
auto test_collector(std::string_view input, bool is_module, KVs&&... kvs) -> sema::Analyzer {
    auto [analyzer, idx] = analyze(input);
    check_errors<sema::Diagnostic>(analyzer.get_diagnostics());
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == sizeof...(KVs));
    CHECK(actual.is_module() == is_module);

    (..., [&] mutable {
        const auto opt = actual.get_opt(kvs.name);
        REQUIRE(opt);

        // The node can be an emplaced type if provided
        if (kvs.node_type_key) {
            const auto type = analyzer.get_pool().get(*kvs.node_type_key);
            REQUIRE(type);
            kvs.node.set_sema_type(*type);
        }
        sema::Symbol expected{kvs.name, &kvs.node};

        // The symbol's upper-level type can be set as well
        if (kvs.symbol_type_key) {
            const auto type = analyzer.get_pool().get(*kvs.symbol_type_key);
            REQUIRE(type);
            expected.emplace_type(*type);
        }
        CHECK(*opt == expected);
    }());
    return std::move(analyzer);
}

// Assumes that the input is not inside of a module
template <typename... KVs>
auto test_collector(std::string_view input, KVs&&... kvs) -> sema::Analyzer {
    return test_collector(input, false, std::forward<KVs>(kvs)...);
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

// A helper for creating non-node symbols
template <typename N> struct TableEntryMaker {
    std::string_view name;
    N (*node_maker)();
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

// A helper for creating leaf-node-based symbols
template <ast::LeafNode N> struct TableEntryMaker<N> {
    std::string_view name;
    N (*node_maker)();
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

template <typename T> struct is_table_node_maker : std::false_type {};
template <ast::LeafNode N> struct is_table_node_maker<TableEntryMaker<N>> : std::true_type {};
template <typename T> constexpr bool is_table_node_maker_v = is_table_node_maker<T>::value;

// Shallowly checks the symbols in the inner scope of a statement
template <typename... Makers>
auto test_hollow_symbols(sema::Analyzer& analyzer, Makers&&... makers) -> void {
    auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(makers));

    (..., [&] {
        const auto  name          = makers.name;
        const auto& expected_node = makers.node_maker();
        if constexpr (is_table_node_maker_v<std::remove_cvref_t<decltype(makers)>>) {
            if (makers.node_type_key) {
                const auto type = analyzer.get_pool().get(*makers.node_type_key);
                REQUIRE(type);
                expected_node.set_sema_type(*type);
            }
        }
        sema::Symbol expected{name, &expected_node};

        if (makers.symbol_type_key) {
            const auto type = analyzer.get_pool().get(*makers.symbol_type_key);
            REQUIRE(type);
            expected.emplace_type(*type);
        }
        CHECK(expected == registry.get_from(1, name));
    }());
}

} // namespace porpoise::tests::helpers

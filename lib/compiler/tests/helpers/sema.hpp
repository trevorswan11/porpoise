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

// A helper for creating non-node symbols
template <typename N> struct TableEntry {
    std::string_view              name;
    N                             node;
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

// A helper for creating leaf-node-based symbols
template <ast::LeafNode N> struct TableEntry<N> {
    std::string_view              name;
    N                             node;
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

template <typename T> struct is_table_node_maker : std::false_type {};
template <ast::LeafNode N> struct is_table_node_maker<TableEntry<N>> : std::true_type {};
template <typename T> constexpr bool is_table_node_maker_v = is_table_node_maker<T>::value;

// A helper for creating declaration symbols
template <> struct TableEntry<ast::DeclStatement> {
    std::string_view              name;
    ast::DeclStatement            node;
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> value_node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

template <typename T> struct is_table_decl_maker : std::false_type {};
template <> struct is_table_decl_maker<TableEntry<ast::DeclStatement>> : std::true_type {};
template <typename T> constexpr bool is_table_decl_maker_v = is_table_decl_maker<T>::value;

template <typename EntryT, typename NodeLike>
auto emplace_type_from_entry(sema::Analyzer& analyzer,
                             const EntryT&   entry,
                             const NodeLike& expected_node) {
    if constexpr (is_table_decl_maker_v<EntryT>) {
        if (entry.value_node_type_key && expected_node.has_value()) {
            const auto type = analyzer.get_pool().get(*entry.value_node_type_key);
            REQUIRE(type);
            expected_node.get_value().set_sema_type(*type);
        }
    } else if constexpr (is_table_node_maker_v<EntryT>) {
        if (entry.node_type_key) {
            const auto type = analyzer.get_pool().get(*entry.node_type_key);
            REQUIRE(type);
            expected_node.set_sema_type(*type);
        }
    }
}

// Verifies that the collectors output matches the provided pairs
template <typename... Entries>
auto test_collector(std::string_view input, bool is_module, Entries&&... entries)
    -> sema::Analyzer {
    auto [analyzer, idx] = analyze(input);
    check_errors<sema::Diagnostic>(analyzer.get_diagnostics());
    const auto& actual = analyzer.get_table(idx);
    CHECK(actual.size() == sizeof...(Entries));
    CHECK(actual.is_module() == is_module);

    (..., [&] mutable {
        const auto opt = actual.get_opt(entries.name);
        REQUIRE(opt);

        // The node can be an emplaced type if provided
        using MakerT = std::remove_cvref_t<decltype(entries)>;
        emplace_type_from_entry<MakerT>(analyzer, entries, entries.node);
        sema::Symbol expected{entries.name, &entries.node};

        // The symbol's upper-level type can be set as well
        if (entries.symbol_type_key) {
            const auto type = analyzer.get_pool().get(*entries.symbol_type_key);
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

// Shallowly checks the symbols in the inner scope of a statement
template <typename... EntryT>
auto test_hollow_symbols(sema::Analyzer& analyzer, EntryT&&... entries) -> void {
    auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(entries));

    (..., [&] {
        const auto  name          = entries.name;
        const auto& expected_node = entries.node;

        using MakerT = std::remove_cvref_t<decltype(entries)>;
        emplace_type_from_entry<MakerT>(analyzer, entries, expected_node);
        sema::Symbol expected{name, &expected_node};

        if (entries.symbol_type_key) {
            const auto type = analyzer.get_pool().get(*entries.symbol_type_key);
            REQUIRE(type);
            expected.emplace_type(*type);
        }
        CHECK(expected == registry.get_from(1, name));
    }());
}

} // namespace porpoise::tests::helpers

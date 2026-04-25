#pragma once

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/ast.hpp" // IWYU pragma: keep

#include "sema/analyzer.hpp"
#include "sema/error.hpp"
#include "sema/module/memory_loader.hpp"
#include "sema/symbol.hpp"

namespace porpoise::tests::helpers {

constexpr std::string_view test_file{"test.porp"};

struct MockFile {
    std::string_view              path;
    std::string_view              source;
    opt::Option<std::string_view> name{};
};

struct SemaTestContext {
    mem::Box<sema::mod::MemoryLoader> loader;
    sema::mod::ModuleManager          manager;
    sema::Analyzer                    analyzer;
    opt::NonNull<sema::mod::Module>   root_mod;

    explicit SemaTestContext(mem::Box<sema::mod::MemoryLoader> mem_loader,
                             const std::vector<MockFile>&      imports,
                             std::string_view                  input);
};

// Collects the assumed-syntactically-valid input and returns the analyzer and parent table index.
[[nodiscard]] auto collect(std::string_view input, const std::vector<MockFile>& imports = {})
    -> std::pair<SemaTestContext, usize>;

// Collects the assumed-syntactically-valid input and checks for no errors
auto collect_and_validate(std::string_view input, const std::vector<MockFile>& imports = {})
    -> std::pair<SemaTestContext, usize>;

// A helper for creating non-node symbols
template <typename N> struct TableEntry {
    std::string_view              name;
    N                             node;
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

// A helper for creating leaf-node-based symbols
template <ast::LeafNode N> struct TableEntry<N> {
    using IsLeaf = void;

    std::string_view              name;
    N                             node;
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

// A helper for creating declaration symbols
template <> struct TableEntry<ast::DeclStatement> {
    using IsDecl = void;

    std::string_view              name;
    ast::DeclStatement            node;
    opt::Option<sema::types::Key> node_type_key{opt::none};
    opt::Option<sema::types::Key> value_node_type_key{opt::none};
    opt::Option<sema::types::Key> symbol_type_key{opt::none};
};

template <typename T>
concept IsDeclEntry = requires { typename T::IsDecl; };

template <typename T>
concept IsLeafEntry = requires { typename T::IsLeaf; };

// A type can be emplaced at different levels depending on template specialization
template <typename EntryT, typename NodeLike>
auto emplace_node_type_from_entry(sema::Analyzer& analyzer,
                                  const EntryT&   entry,
                                  const NodeLike& expected_node) {
    if constexpr (IsLeafEntry<EntryT>) {
        if (entry.node_type_key) {
            const auto type = analyzer.get_pool().get(*entry.node_type_key);
            REQUIRE(type);
            expected_node.set_sema_type(*type);
        }
    } else if constexpr (IsDeclEntry<EntryT>) {
        if (entry.value_node_type_key && expected_node.has_value()) {
            const auto type = analyzer.get_pool().get(*entry.value_node_type_key);
            REQUIRE(type);
            expected_node.get_value().set_sema_type(*type);
        }
    }
}

// The symbol's upper-level type can be set by optional configuration in the entry
template <typename EntryT>
auto emplace_symbol_type_from_entry(sema::Analyzer& analyzer,
                                    const EntryT&   entry,
                                    sema::Symbol&   expected_symbol) {
    if (entry.symbol_type_key) {
        const auto type = analyzer.get_pool().get(*entry.symbol_type_key);
        REQUIRE(type);
        expected_symbol.emplace_type(*type);
    }
}

// Verifies that the collectors output matches the provided pairs
template <typename... Entries>
auto test_collector(std::string_view             input,
                    const std::vector<MockFile>& imports,
                    Entries&&... entries) -> SemaTestContext {
    auto [ctx, idx]      = collect_and_validate(input, imports);
    auto&       analyzer = ctx.analyzer;
    const auto& actual   = analyzer.get_table(idx);
    CHECK(actual.size() == sizeof...(Entries));

    (..., [&] mutable {
        const auto opt = actual.get_opt(entries.name);
        REQUIRE(opt);

        using MakerT = std::remove_cvref_t<decltype(entries)>;
        emplace_node_type_from_entry<MakerT>(analyzer, entries, entries.node);

        opt::Option<sema::Symbol> expected;
        if constexpr (std::is_same_v<sema::SymbolicImport, decltype(entries.node)>) {
            expected.emplace(entries.name, entries.node);
        } else {
            expected.emplace(entries.name, &entries.node);
        }
        REQUIRE(expected);

        emplace_symbol_type_from_entry(analyzer, entries, *expected);
        CHECK(*opt == *expected);
    }());
    return std::move(ctx);
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view             failing,
                         const std::vector<MockFile>& imports,
                         Ds&&... expected_diagnostics) -> void {
    const std::array expected_arr{std::forward<Ds>(expected_diagnostics)...};
    const auto       expected_count = sizeof...(Ds);

    auto [ctx, idx] = collect(failing, imports);
    auto test_mod   = ctx.root_mod;

    REQUIRE(test_mod->has_sema_diagnostics());
    const auto& errors = test_mod->get_sema_diagnostics();

    if (errors.size() != expected_count) {
        for (const auto& e : errors) { fmt::println("{}", e); }
        CHECK(errors.size() == expected_count);
    }
    CHECK(std::ranges::equal(errors, expected_arr));
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    test_collector_fail(failing, {}, std::forward<Ds>(expected_diagnostics)...);
}

// A common decl for tests formatted as `const name := assign;`
auto        common_decl(std::string_view name, std::string_view assign) -> ast::DeclStatement;
inline auto foo_bar_decl() -> ast::DeclStatement { return common_decl("foo", "bar"); }

// Shallowly checks the symbols in the inner scope of a statement
template <typename... Entries>
auto test_hollow_symbols(SemaTestContext& ctx, Entries&&... entries) -> void {
    auto& analyzer = ctx.analyzer;
    auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    CHECK(registry.get(1).size() == sizeof...(entries));

    (..., [&] {
        const auto  name          = entries.name;
        const auto& expected_node = entries.node;

        using MakerT = std::remove_cvref_t<decltype(entries)>;
        emplace_node_type_from_entry<MakerT>(analyzer, entries, expected_node);
        sema::Symbol expected{name, &expected_node};

        emplace_symbol_type_from_entry(analyzer, entries, expected);
        CHECK(expected == registry.get_from(1, name));
    }());
}

} // namespace porpoise::tests::helpers

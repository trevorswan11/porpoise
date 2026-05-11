#pragma once

#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "helpers/ast.hh" // IWYU pragma: keep

#include "sema/analyzer.hh"
#include "sema/error.hh"
#include "sema/symbol.hh" // IWYU pragma: keep

#include "module/memory_loader.hh"

namespace porpoise::tests::helpers {

constexpr std::string_view TEST_FILENAME{"test.porp"};

struct MockFile {
    std::string_view              path;
    std::string_view              source;
    opt::Option<std::string_view> name{};
};

// Tests the collected state of a minimal non-public implicit declaration
auto test_common_decl_collection(const sema::SymbolTableRegistry& registry,
                                 const mod::Module&               module,
                                 usize                            idx,
                                 std::string_view                 name = "foo") -> void;

struct SemaTestContext {
    mem::Box<mod::MemoryLoader> loader;
    mod::ModuleManager          manager;
    sema::Analyzer              analyzer;
    mem::NonNull<mod::Module>   root_mod;

    // The root is automatically added to the internal loader and can be immediately analyzed
    explicit SemaTestContext(const std::vector<MockFile>& imports,
                             const std::filesystem::path& root_path,
                             std::string_view             input,
                             std::ostream&                error_stream = std::cerr);

    // Tests the collected state of a minimal non-public implicit declaration in the context
    auto test_common_decl_collection(usize idx, std::string_view name = "foo") -> void;
};

// Collects the assumed-syntactically-valid input and returns the analyzer and parent table index.
[[nodiscard]] auto collect(std::string_view input, const std::vector<MockFile>& imports = {})
    -> std::pair<SemaTestContext, usize>;

// Collects the assumed-syntactically-valid input and checks for no errors
auto collect_and_check(std::string_view input, const std::vector<MockFile>& imports = {})
    -> std::pair<SemaTestContext, usize>;

// Runs the entire Analyzer on the provided input without checking semantic validity (no errors)
template <typename... Mocks>
    requires(std::same_as<Mocks, MockFile> && ...)
auto analyze_unchecked(std::string_view root_path,
                       std::ostream&    error_stream,
                       std::string_view input,
                       Mocks&&... mocks) -> SemaTestContext {
    SemaTestContext ctx{helpers::make_vector<MockFile>(std::forward<Mocks>(mocks)...),
                        root_path,
                        input,
                        error_stream};
    REQUIRE_FALSE(ctx.root_mod->has_parser_diagnostics());

    auto& analyzer = ctx.analyzer;
    REQUIRE(analyzer.analyze(root_path));
    return ctx;
}

// Runs the entire Analyzer on the provided input and checks for errors
template <typename... Mocks>
    requires(std::same_as<Mocks, MockFile> && ...)
auto analyze(std::string_view root_path, std::string_view input, Mocks&&... mocks)
    -> SemaTestContext {
    auto ctx = analyze_unchecked(root_path, std::cerr, input, std::forward<Mocks>(mocks)...);
    REQUIRE_FALSE(ctx.root_mod->has_sema_diagnostics());
    return ctx;
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view             failing,
                         const std::vector<MockFile>& imports,
                         Ds&&... expected_diagnostics) -> void {
    auto [ctx, idx] = collect(failing, imports);
    auto test_mod   = ctx.root_mod;

    REQUIRE(test_mod->has_sema_diagnostics());
    const auto& errors = test_mod->get_sema_diagnostics();
    helpers::check_errors_against<sema::Diagnostic>(errors,
                                                    std::forward<Ds>(expected_diagnostics)...);
}

// Tests a semantically failing input against the expected generated errors
template <typename... Ds>
    requires(std::same_as<Ds, sema::Diagnostic> && ...)
auto test_collector_fail(std::string_view failing, Ds&&... expected_diagnostics) -> void {
    test_collector_fail(failing, {}, std::forward<Ds>(expected_diagnostics)...);
}

} // namespace porpoise::tests::helpers

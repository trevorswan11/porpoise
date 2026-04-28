#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

using MockFile = helpers::MockFile;

constexpr std::string_view a_porp{R"(module; import "b.porp" as b;)"};
constexpr std::string_view b_porp{R"(module; import "a.porp" as a;)"};

TEST_CASE("Circular imports") {
    constexpr std::string_view root{"a.porp"};

    auto        ctx      = helpers::analyze(root, a_porp, MockFile{"b.porp", b_porp});
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 2);

    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "a"));
}

constexpr std::string_view importer_porp{R"(
import "a.porp" as a;
import "b.porp" as b;
)"};

constexpr std::string_view diamond{R"(import std;)"};
constexpr std::string_view std_porp{R"(module;)"};

TEST_CASE("Diamond dependencies") {
    constexpr std::string_view root{"main.porp"};

    auto        ctx      = helpers::analyze(root,
                                importer_porp,
                                MockFile{"a.porp", diamond},
                                MockFile{"b.porp", diamond},
                                MockFile{"std.porp", std_porp, "std"});
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 4);

    CHECK(registry.get_from_opt(0, "a"));
    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "std"));
    CHECK(registry.get(2).empty());
    CHECK(registry.get_from_opt(3, "std"));
}

constexpr std::string_view self_porp{R"(import "self.porp" as self;)"};

TEST_CASE("Self import") {
    constexpr std::string_view root{"self.porp"};

    auto        ctx      = helpers::analyze(root, self_porp);
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 1);
    CHECK(registry.get_from_opt(0, "self"));
}

TEST_CASE("Unknown file module") {
    auto ctx = helpers::analyze_unchecked(helpers::test_file, R"(import "a.porp" as a;)");
    REQUIRE(ctx.root_mod->has_sema_diagnostics());
    helpers::check_errors_against<sema::Diagnostic>(
        ctx.root_mod->get_sema_diagnostics(),
        sema::Diagnostic{R"(Could not load file: "a.porp")",
                         sema::Error::PATH_DOES_NOT_EXIST,
                         std::pair{0uz, 7uz}});
}

} // namespace porpoise::tests

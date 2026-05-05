#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mut      = helpers::mut;

using MockFile = helpers::MockFile;

TEST_CASE("Import aliases correctly used") {
    helpers::test_collector(
        R"(import foo as A; import "f.porp" as F; const foo := bar;)",
        helpers::make_vector<MockFile>(MockFile{"foo.porp", "const foo := bar;", "foo"},
                                       MockFile{"f.porp", "const foo := bar;"}),
        helpers::TableEntry<ast::ImportStatement>{
            "A",
            ast::ImportStatement{
                syntax::Token{keywords::IMPORT},
                ast::LibraryImport{helpers::make_ident("foo"), helpers::make_ident<true>("A")}},
            sema::types::Key{sema::TypeKind::MODULE, mut::IMMUTABLE, 1},
            sema::types::Key{sema::TypeKind::MODULE, mut::IMMUTABLE, 1}},
        helpers::TableEntry<ast::ImportStatement>{
            "F",
            ast::ImportStatement{
                syntax::Token{keywords::IMPORT},
                ast::FileImport{helpers::make_primitive<ast::StringExpression>(R"("f.porp")"),
                                helpers::make_ident("F")}},
            sema::types::Key{sema::TypeKind::MODULE, mut::IMMUTABLE, 2},
            sema::types::Key{sema::TypeKind::MODULE, mut::IMMUTABLE, 2}},
        helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Public import query") {
    const auto test = [](bool is_public) {
        const sema::types::Key key{sema::TypeKind::MODULE, mut::IMMUTABLE, 1};
        auto                   ctx = helpers::test_collector(
            fmt::format("{} import std;", is_public ? "pub" : ""),
            helpers::make_vector<MockFile>(MockFile{"std.porp", "var a: i32;", "std"}),
            helpers::TableEntry<ast::ImportStatement>{
                "std",
                ast::ImportStatement{syntax::Token{is_public ? keywords::PUBLIC : keywords::IMPORT},
                                     ast::LibraryImport{helpers::make_ident("std"), {}}},
                key,
                key});

        auto&       table      = ctx.analyzer.get_table(*ctx.root_mod->root_table_idx);
        const auto& std_import = table.get_opt("std");
        REQUIRE(std_import);
        CHECK(std_import->is_public() == is_public);
    };

    test(true);
    test(false);
}

constexpr std::string_view a_porp{R"(pub import "b.porp" as b;)"};
constexpr std::string_view b_porp{R"(pub import "a.porp" as a;)"};

TEST_CASE("Circular imports") {
    constexpr std::string_view root{"a.porp"};

    auto        ctx      = helpers::analyze(root, a_porp, MockFile{"b.porp", b_porp});
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 3);

    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "a"));
}

constexpr std::string_view importer_porp{R"(
import "a.porp" as a;
import "b.porp" as b;
)"};

constexpr std::string_view diamond{R"(import std;)"};
constexpr std::string_view std_porp{R"(pub import "io.porp" as io;)"};

TEST_CASE("Diamond dependencies") {
    constexpr std::string_view root{"main.porp"};

    auto        ctx      = helpers::analyze(root,
                                importer_porp,
                                MockFile{"a.porp", diamond},
                                MockFile{"b.porp", diamond},
                                MockFile{"std.porp", std_porp, "std"});
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 5);

    CHECK(registry.get_from_opt(0, "a"));
    CHECK(registry.get_from_opt(0, "b"));
    CHECK(registry.get_from_opt(1, "std"));
    CHECK(registry.get_from_opt(2, "io"));
    CHECK(registry.get_from_opt(3, "std"));
}

constexpr std::string_view self_porp{R"(import "self.porp" as self;)"};

TEST_CASE("Self import") {
    constexpr std::string_view root{"self.porp"};

    auto        ctx      = helpers::analyze(root, self_porp);
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 2);
    CHECK(registry.get_from_opt(0, "self"));
}

TEST_CASE("Unknown file module") {
    std::stringstream ss;
    auto ctx = helpers::analyze_unchecked(helpers::TEST_FILENAME, ss, R"(import "a.porp" as a;)");
    REQUIRE(ctx.root_mod->has_sema_diagnostics());

    constexpr std::string_view expected{
        R"(test.porp:1:8: error: Could not find path 'a.porp' in virtual file system
    import "a.porp" as a;
           ^
)"};
    CHECK(ss.view() == expected);
}

} // namespace porpoise::tests

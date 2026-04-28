#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

using MockFile = helpers::MockFile;

TEST_CASE("Import aliases correctly used") {
    const ast::ImportStatement import_one{
        syntax::Token{keywords::IMPORT},
        ast::LibraryImport{helpers::make_ident("foo"), helpers::make_ident<true>("A")}};
    const ast::ImportStatement import_two{
        syntax::Token{keywords::IMPORT},
        ast::FileImport{helpers::make_primitive<ast::StringExpression>(R"("f.porp")"),
                        helpers::make_ident("F")}};

    helpers::test_collector(
        R"(import foo as A; import "f.porp" as F; const foo := bar;)",
        helpers::make_vector<MockFile>(MockFile{"foo.porp", "const foo := bar;", "foo"},
                                       MockFile{"f.porp", "const foo := bar;"}),
        helpers::TableEntry{"A", sema::SymbolicImport{&import_one, opt::none}},
        helpers::TableEntry{"F", sema::SymbolicImport{&import_two, opt::none}},
        helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Public import query") {
    const auto test = [](bool is_public) {
        const auto                 input = fmt::format("{} import std;", is_public ? "pub" : "");
        const ast::ImportStatement import_stmt{
            syntax::Token{is_public ? keywords::PUBLIC : keywords::IMPORT},
            ast::LibraryImport{helpers::make_ident("std"), {}}};

        auto ctx = helpers::test_collector(
            input,
            helpers::make_vector<MockFile>(MockFile{"std.porp", "var a: i32;", "std"}),
            helpers::TableEntry{"std", sema::SymbolicImport{&import_stmt, opt::none}});

        auto&       table      = ctx.analyzer.get_table(ctx.root_mod->root_table_idx);
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
    REQUIRE(registry.size() == 2);

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
    REQUIRE(registry.size() == 4);

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
    REQUIRE(registry.size() == 1);
    CHECK(registry.get_from_opt(0, "self"));
}

TEST_CASE("Unknown file module") {
    auto ctx = helpers::analyze_unchecked(helpers::test_file, R"(import "a.porp" as a;)");
    REQUIRE(ctx.root_mod->has_sema_diagnostics());

    const auto& diagnostics = ctx.root_mod->get_sema_diagnostics();
    helpers::check_errors_against<sema::Diagnostic>(
        diagnostics,
        sema::Diagnostic{R"(Could not find path 'a.porp' in virtual file system)",
                         sema::Error::PATH_DOES_NOT_EXIST,
                         std::pair{0uz, 7uz}});

    SECTION("Proper module diagnostic formatting") {
        REQUIRE(diagnostics.size() == 1);
        std::stringstream ss;
        detail::format_module_diagnostic(ss, diagnostics[0].to_formattable(), *ctx.root_mod, false);

        constexpr std::string_view expected{
            R"(test.porp:1:8: error: Could not find path 'a.porp' in virtual file system
    import "a.porp" as a;
           ^)"};
        CHECK(ss.view() == expected);
    }
}

} // namespace porpoise::tests

#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

using MockFile = helpers::MockFile;

TEST_CASE("Module visibility modifier") {
    const auto test = [](bool is_module) {
        const auto input = fmt::format("{}import std; using I = i32;", is_module ? "module;" : "");
        const ast::ImportStatement import_stmt{syntax::Token{keywords::IMPORT},
                                               ast::LibraryImport{helpers::make_ident("std"), {}}};

        auto ctx = helpers::test_collector(
            input,
            helpers::make_vector<MockFile>(MockFile{"std.porp", "var a: i32;", "std"}),
            helpers::TableEntry{"std",
                                [&import_stmt, is_module] {
                                    if (is_module) { import_stmt.mark_public(); }
                                    return sema::SymbolicImport{&import_stmt, opt::none};
                                }()},
            helpers::TableEntry{"I", [is_module] {
                                    ast::UsingStatement using_stmt{syntax::Token{keywords::USING},
                                                                   helpers::make_ident("I"),
                                                                   ast::ExplicitType{
                                                                       mods::BASE,
                                                                       helpers::make_ident("i32"),
                                                                   }};
                                    if (is_module) { using_stmt.mark_public(); }
                                    return using_stmt;
                                }()});

        CHECK(ctx.analyzer.get_table(ctx.root_mod->root_table_idx).is_module() == is_module);
    };

    test(true);
    test(false);
}

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

TEST_CASE("Public modifiers and querying") {
    auto [ctx, idx] = helpers::collect("module; import foo; using bar = baz; pub const a := 2;");

    auto&          analyzer     = ctx.analyzer;
    auto&          parent_table = analyzer.get_table(idx);
    constexpr auto names        = std::array{"foo", "bar", "a"};

    for (const auto& name : names) {
        auto symbol = parent_table.get_opt(name);
        REQUIRE(symbol);
        CHECK(symbol->is_public());
    }
}

TEST_CASE("Duplicate module declaration") {
    helpers::test_collector_fail("module; module;",
                                 sema::Diagnostic{"Only one module statement is allowed per file",
                                                  sema::Error::DUPLICATE_MODULE_STATEMENT,
                                                  std::pair{0uz, 8uz}});
}

TEST_CASE("Illegal module statement location") {
    helpers::test_collector_fail(
        "const a := 2; module;",
        sema::Diagnostic{"Module indicator must be first statement of file",
                         sema::Error::ILLEGAL_MODULE_STATEMENT_LOCATION,
                         std::pair{0uz, 14uz}});
}

} // namespace porpoise::tests

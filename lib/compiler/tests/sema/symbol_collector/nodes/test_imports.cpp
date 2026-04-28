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

} // namespace porpoise::tests

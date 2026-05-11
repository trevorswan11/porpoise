#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

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

} // namespace porpoise::tests

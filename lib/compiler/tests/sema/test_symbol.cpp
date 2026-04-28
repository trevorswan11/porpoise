#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "sema/symbol.hpp"

namespace porpoise::tests {

TEST_CASE("Basic table operations") {
    sema::SymbolTable          table;
    const std::string_view     name{"a"};
    const ast::ImportStatement import_node{syntax::Token{syntax::keywords::IMPORT},
                                           ast::LibraryImport{helpers::make_ident(name), {}}};
    const sema::SymbolicImport import_sym{&import_node, opt::none};

    CHECK(table.empty());
    CHECK(table.insert(name, import_sym));
    CHECK(table.size() == 1);
    CHECK_FALSE(table.empty());

    CHECK(table.has(name));
    const auto& retrieved = table.get_opt(name);
    CHECK(retrieved);
    CHECK(retrieved->get_name() == name);
    CHECK_FALSE(retrieved->has_type());
    CHECK(retrieved->is_import_stmt());
    CHECK(retrieved->get_import_stmt() == import_sym);

    CHECK(*retrieved == table.get(name));
}

TEST_CASE("Multiple table import") {
    sema::SymbolTable          table;
    const std::string_view     library_name{"a"};
    const ast::ImportStatement library_import{
        syntax::Token{syntax::keywords::IMPORT},
        ast::LibraryImport{helpers::make_ident(library_name), {}}};
    const sema::SymbolicImport library_sym{&library_import, opt::none};
    CHECK(table.insert(library_name, library_sym));

    const std::string_view     user_name{"node"};
    const std::string          user_file{"node.p"};
    const ast::ImportStatement user_import{
        syntax::Token{syntax::keywords::IMPORT},
        ast::FileImport{helpers::make_primitive<ast::StringExpression>(user_file),
                        helpers::make_ident(user_name)}};
    const sema::SymbolicImport user_sym{&user_import, opt::none};
    CHECK(table.insert(user_name, user_sym));

    CHECK(table.size() == 2);
    CHECK(table.has(library_name));
    CHECK(table.get(library_name) == sema::Symbol{library_name, library_sym});
    CHECK(table.has(user_name));
    CHECK(table.get(user_name) == sema::Symbol{user_name, user_sym});

    CHECK_FALSE(table.has("b"));
    CHECK_FALSE(table.get_opt("b"));
}

TEST_CASE("Duplicate table inserts") {
    sema::SymbolTable          table;
    const std::string_view     name{"a"};
    const ast::ImportStatement import_node{syntax::Token{syntax::keywords::IMPORT},
                                           ast::LibraryImport{helpers::make_ident(name), {}}};
    const sema::SymbolicImport import_sym{&import_node, opt::none};

    CHECK(table.insert(name, import_sym));
    CHECK_FALSE(table.insert(name, import_sym));
    CHECK(table.size() == 1);
}

TEST_CASE("Illegal registry insert") {
    sema::SymbolTableRegistry  registry;
    const ast::ImportStatement import_node{syntax::Token{syntax::keywords::IMPORT},
                                           ast::LibraryImport{helpers::make_ident("a"), {}}};
    const sema::SymbolicImport import_sym{&import_node, opt::none};
    const auto                 result = registry.insert_into(0, "a", import_sym);

    CHECK_FALSE(result);
    CHECK(result.error() == sema::Diagnostic{sema::Error::INVALID_TABLE_IDX});
}

TEST_CASE("Safety checked registry operations") {
    sema::SymbolTableRegistry registry;
    CHECK_THROWS(registry.get(1));
    CHECK_FALSE(registry.get_opt(1));
    CHECK_THROWS(registry.get_from(1, "a"));
    CHECK_FALSE(registry.get_from_opt(1, "a"));
}

} // namespace porpoise::tests

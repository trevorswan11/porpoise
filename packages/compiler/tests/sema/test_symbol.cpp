#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "sema/symbol.hpp"

#include "syntax/keywords.hpp"
#include "syntax/token.hpp"

#include "ast/ast.hpp"

namespace porpoise::tests {

TEST_CASE("Basic table operations") {
    sema::SymbolTable          table;
    const std::string_view     name{"a"};
    const ast::ImportStatement import_node{syntax::Token{syntax::keywords::IMPORT},
                                           ast::ModuleImport{helpers::make_ident(name), {}}};
    CHECK(table.empty());
    CHECK(table.insert(name, &import_node));
    CHECK(table.size() == 1);
    CHECK_FALSE(table.empty());

    CHECK(table.has(name));
    const auto& retrieved = table.get_opt(name);
    CHECK(retrieved);
    CHECK(retrieved->get_name() == name);
    CHECK_FALSE(retrieved->has_type());
    CHECK(retrieved->is_import_stmt());
    CHECK(retrieved->get_import_stmt() == import_node);

    CHECK(*retrieved == table.get(name));
}

TEST_CASE("Multiple table import") {
    sema::SymbolTable          table;
    const std::string_view     module_name{"a"};
    const ast::ImportStatement module_import{
        syntax::Token{syntax::keywords::IMPORT},
        ast::ModuleImport{helpers::make_ident(module_name), {}}};
    CHECK(table.insert(module_name, &module_import));

    const std::string_view     user_name{"node"};
    const std::string          user_file{"node.p"};
    const ast::ImportStatement user_import{
        syntax::Token{syntax::keywords::IMPORT},
        ast::UserImport{make_box<ast::StringExpression>(
                            syntax::Token{syntax::TokenType::STRING, user_file}, user_file),
                        helpers::make_ident(user_name)}};
    CHECK(table.insert(user_name, &user_import));

    CHECK(table.size() == 2);
    CHECK(table.has(module_name));
    CHECK(table.get(module_name) == sema::Symbol{module_name, &module_import});
    CHECK(table.has(user_name));
    CHECK(table.get(user_name) == sema::Symbol{user_name, &user_import});

    CHECK_FALSE(table.has("b"));
    CHECK_FALSE(table.get_opt("b"));
}

TEST_CASE("Duplicate table inserts") {
    sema::SymbolTable          table;
    const std::string_view     name{"a"};
    const ast::ImportStatement import_node{syntax::Token{syntax::keywords::IMPORT},
                                           ast::ModuleImport{helpers::make_ident(name), {}}};

    CHECK(table.insert(name, &import_node));
    CHECK_FALSE(table.insert(name, &import_node));
    CHECK(table.size() == 1);
}

} // namespace porpoise::tests

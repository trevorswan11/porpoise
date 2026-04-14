#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

TEST_CASE("Module visibility modifier") {
    const auto test = [](bool is_module) {
        const auto input =
            fmt::format("{}module;\nimport std; using Integer = i32;", is_module ? "" : "//");

        helpers::test_collector(
            input,
            is_module,
            std::pair{"std",
                      [is_module]() {
                          ast::ImportStatement import_stmt{
                              syntax::Token{keywords::IMPORT},
                              ast::LibraryImport{helpers::make_ident("std"), {}}};
                          if (is_module) { import_stmt.mark_public(); }
                          return import_stmt;
                      }()},
            std::pair{"Integer", [is_module] {
                          ast::UsingStatement using_stmt{syntax::Token{keywords::USING},
                                                         helpers::make_ident("Integer"),
                                                         ast::ExplicitType{
                                                             mods::BASE,
                                                             helpers::make_ident("i32"),
                                                         }};
                          if (is_module) { using_stmt.mark_public(); }
                          return using_stmt;
                      }()});
    };

    test(true);
    test(false);
}

TEST_CASE("Import aliases correctly used") {
    helpers::test_collector(
        "import foo as A; const foo := bar;",
        false,
        std::pair{"A",
                  ast::ImportStatement{syntax::Token{keywords::IMPORT},
                                       ast::LibraryImport{helpers::make_ident("foo"),
                                                          helpers::make_ident<true>("A")}}},
        std::pair{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Duplicate module declaration") {
    helpers::test_collector_fail("module; module;",
                                 sema::Diagnostic{"Only one module statement is allowed per file",
                                                  sema::Error::DUPLICATE_MODULE_STATEMENT,
                                                  std::pair{1uz, 9uz}});
}

} // namespace porpoise::tests

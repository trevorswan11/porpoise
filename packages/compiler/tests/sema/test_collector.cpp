#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"
#include "helpers/sema.hpp"

#include "syntax/keywords.hpp"

#include "ast/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;
namespace mods     = helpers::type_modifiers;

namespace helpers {

auto test_illegal_top_level_stmt(std::string_view input, std::string_view stringified) -> void {
    helpers::test_collector_fail(
        input,
        sema::SemaDiagnostic{fmt::format("Cannot have {} at the top level", stringified),
                             sema::SemaError::ILLEGAL_TOP_LEVEL_STATEMENT,
                             std::pair{1uz, 1uz}});
}

} // namespace helpers

TEST_CASE("Holistic language example") {
    const auto test = [](bool is_module) {
        const auto input = fmt::format(R"({}module;
                                        import std;
                                        using Integer = int;
                                        const a: Integer = 1;)",
                                       is_module ? "" : "//");
        helpers::test_collector(
            input,
            is_module,
            std::pair{"std",
                      ast::ImportStatement{syntax::Token{keywords::IMPORT},
                                           ast::ModuleImport{helpers::make_ident("std"), {}}}},
            std::pair{"Integer",
                      ast::UsingStatement{syntax::Token{keywords::USING},
                                          helpers::make_ident("Integer"),
                                          ast::ExplicitType{
                                              mods::BASE,
                                              helpers::make_ident("int"),
                                          }}},
            std::pair{
                "a",
                ast::DeclStatement{
                    syntax::Token{keywords::CONST},
                    helpers::make_ident("a"),
                    make_box<ast::TypeExpression>(syntax::Token{syntax::TokenType::COLON, ":"},
                                                  ast::ExplicitType{
                                                      mods::BASE,
                                                      helpers::make_ident("Integer"),
                                                  }),
                    make_box<ast::SignedIntegerExpression>(
                        syntax::Token{syntax::TokenType::INT_10, "1"}, 1),
                    ast::DeclModifiers::CONSTANT,
                }});
    };

    test(true);
    test(false);
}

TEST_CASE("Duplicate module declaration") {
    helpers::test_collector_fail(
        "module; module;",
        sema::SemaDiagnostic{"Only one module statement is allowed per file",
                             sema::SemaError::DUPLICATE_MODULE_STATEMENT,
                             std::pair{1uz, 9uz}});
}

TEST_CASE("Illegal module location") {
    helpers::test_collector_fail(
        "const a := 2; module;",
        sema::SemaDiagnostic{"Module indicator must be first statement of file",
                             sema::SemaError::ILLEGAL_MODULE_STATEMENT_LOCATION,
                             std::pair{1uz, 15uz}});
}

TEST_CASE("Illegal top-level statement") {
    helpers::test_illegal_top_level_stmt("{}", "block");
    helpers::test_illegal_top_level_stmt("defer 2;", "defer");
    helpers::test_illegal_top_level_stmt("_ = 2;", "discard");
    helpers::test_illegal_top_level_stmt("2;", "expression");
    helpers::test_illegal_top_level_stmt("return 2;", "jump");
}

} // namespace porpoise::tests

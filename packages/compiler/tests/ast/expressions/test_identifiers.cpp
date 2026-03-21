#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace helpers {

auto test_ident(std::string_view input) -> void {
    test_expr_stmt(input, ident_from(trim_semicolons(input)));
}

} // namespace helpers

TEST_CASE("Normal identifiers") {
    helpers::test_ident("foobar;");
    helpers::test_ident("int;");
    helpers::test_ident("uint;");
    helpers::test_ident("float;");
    helpers::test_ident("byte;");
    helpers::test_ident("string;");
    helpers::test_ident("bool;");
    helpers::test_ident("void;");
}

TEST_CASE("Non-terminated identifier") {
    helpers::test_parser_fail(
        "foobar",
        syntax::ParserDiagnostic{
            "Expected token SEMICOLON, found END", syntax::ParserError::UNEXPECTED_TOKEN, 1, 7});
}

TEST_CASE("Builtin identifiers") {
    for (const auto& [str, tok] : syntax::ALL_BUILTINS) {
        helpers::test_ident(fmt::format("{};", str));
    }
}

} // namespace porpoise::tests

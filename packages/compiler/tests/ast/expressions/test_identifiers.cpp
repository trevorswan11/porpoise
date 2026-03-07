#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/helpers.hpp"

#include "ast/expressions/identifier.hpp"

#include "lexer/keywords.hpp"

namespace conch::tests {

namespace helpers {

auto test_ident(std::string_view input, Optional<TokenType> expected_type = TokenType::IDENT)
    -> void {
    const Token start_token{*expected_type, trim_semicolons(input)};
    test_stmt(
        input,
        ast::ExpressionStatement{start_token, make_box<ast::IdentifierExpression>(start_token)});
}

} // namespace helpers

TEST_CASE("Normal identifiers") {
    helpers::test_ident("foobar;");
    helpers::test_ident("int;", TokenType::INT_TYPE);
    helpers::test_ident("uint;", TokenType::UINT_TYPE);
    helpers::test_ident("float;", TokenType::FLOAT_TYPE);
    helpers::test_ident("byte;", TokenType::BYTE_TYPE);
    helpers::test_ident("string;", TokenType::STRING_TYPE);
    helpers::test_ident("bool;", TokenType::BOOL_TYPE);
    helpers::test_ident("void;", TokenType::VOID_TYPE);
}

TEST_CASE("Builtin identifiers") {
    for (const auto& [str, tok] : ALL_BUILTINS) {
        helpers::test_ident(fmt::format("{};", str), tok);
    }
}

} // namespace conch::tests

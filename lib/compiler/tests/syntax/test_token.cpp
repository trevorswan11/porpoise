#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>

#include "syntax/token.hpp"

#include "variant.hpp"

namespace porpoise::tests {

using namespace syntax;

namespace helpers {

auto test_token_promotion(std::string_view                           input,
                          TokenType                                  type,
                          std::variant<std::string_view, TokenError> expected) -> void {
    const Token tok{type, input, 0, 0};
    const auto  promoted = tok.promote();

    std::visit(Overloaded{[&promoted](const std::string_view& string) {
                              CHECK(promoted);
                              CHECK(*promoted == string);
                          },
                          [&promoted](const TokenError& error) {
                              CHECK_FALSE(promoted);
                              CHECK(promoted.error() == TokenDiagnostic{error, 0, 0});
                          }},
               expected);
}

auto test_string(std::string_view input, std::variant<std::string_view, TokenError> expected) {
    test_token_promotion(input, TokenType::STRING, expected);
}

auto test_ml_string(std::string_view input, std::variant<std::string_view, TokenError> expected) {
    test_token_promotion(input, TokenType::MULTILINE_STRING, expected);
}

} // namespace helpers

TEST_CASE("Promotion of invalid tokens") {
    helpers::test_token_promotion("1", TokenType::INT_10, TokenError::NON_STRING_TOKEN);
}

TEST_CASE("Promotion of standard string literals") {
    helpers::test_string(R"("Hello, World!")", "Hello, World!");
    helpers::test_string(R"(""Hello, World!"")", R"("Hello, World!")");
    helpers::test_string(R"("")", "");
}

TEST_CASE("Malformed string literal") {
    helpers::test_token_promotion(R"(")", TokenType::STRING, TokenError::UNEXPECTED_CHAR);
}

TEST_CASE("Promotion of multiline literals") {
    helpers::test_ml_string(R"(\\Hello,"World!")", R"(Hello,"World!")");
    helpers::test_ml_string("\\\\Hello,\n\\\\World!\n\\\\", "Hello,\nWorld!\n");
    helpers::test_ml_string(R"(\\)", "");
}

TEST_CASE("Token formatting") {
    const auto expected{R"(STRING("Hello, World!") [1, 24])"};
    const auto actual = fmt::format("{}", Token{TokenType::STRING, R"("Hello, World!")", 1, 24});
    CHECK(expected == actual);
}

} // namespace porpoise::tests

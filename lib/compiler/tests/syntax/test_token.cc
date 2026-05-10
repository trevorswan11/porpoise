#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>

#include "syntax/token.hh"

namespace porpoise::tests {

using syntax::TokenType;

namespace helpers {

auto test_token_promotion(std::string_view input, TokenType type, std::string_view expected)
    -> void {
    const syntax::Token tok{type, input, 0, 0};
    const auto          promoted = tok.materialize_string();
    CHECK(promoted == expected);
}

auto test_string(std::string_view input, std::string_view expected) {
    test_token_promotion(input, TokenType::STRING, expected);
}

auto test_ml_string(std::string_view input, std::string_view expected) {
    test_token_promotion(input, TokenType::MULTILINE_STRING, expected);
}

} // namespace helpers

TEST_CASE("Promotion of standard string literals") {
    helpers::test_string(R"("Hello, World!")", "Hello, World!");
    helpers::test_string(R"(""Hello, World!"")", R"("Hello, World!")");
    helpers::test_string(R"("")", "");
}

TEST_CASE("Promotion of multiline literals") {
    helpers::test_ml_string(R"(\\Hello,"World!")", R"(Hello,"World!")");
    helpers::test_ml_string("\\\\Hello,\n\\\\World!\n\\\\", "Hello,\nWorld!\n");
    helpers::test_ml_string(R"(\\)", "");
}

} // namespace porpoise::tests

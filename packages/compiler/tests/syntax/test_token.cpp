#include <string>

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>

#include "syntax/token.hpp"

namespace porpoise::tests {

using namespace syntax;

TEST_CASE("Promotion of invalid tokens") {
    const auto  input{"1"};
    const Token tok{TokenType::INT_10, input, 0, 0};

    const auto promoted = tok.promote();
    CHECK_FALSE(promoted);
    CHECK(promoted.error() == Diagnostic{TokenError::NON_STRING_TOKEN, 0, 0});
}

TEST_CASE("Promotion of standard string literals") {
    SECTION("Normal case") {
        const auto  input{R"("Hello, World!")"};
        const Token tok = {TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const auto expected{"Hello, World!"};
        CHECK(expected == *promoted);
    }

    SECTION("Escaped case") {
        const auto  input{R"(""Hello, World!"")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const auto expected{R"("Hello, World!")"};
        CHECK(expected == *promoted);
    }

    SECTION("Empty case") {
        const auto  input{R"("")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const std::string expected;
        CHECK(expected == *promoted);
    }

    SECTION("Malformed case") {
        const auto  input{R"(")"};
        const Token tok{TokenType::STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK_FALSE(promoted);
        CHECK(promoted.error() == Diagnostic{TokenError::UNEXPECTED_CHAR, 0, 0});
    }
}

TEST_CASE("Promotion of multiline literals") {
    SECTION("Normal case no newline") {
        const auto  input{R"(\\Hello,"World!")"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const auto expected = R"(Hello,"World!")";
        CHECK(expected == *promoted);
    }

    SECTION("Normal case newline") {
        const auto  input{"\\\\Hello,\n\\\\World!\n\\\\"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const auto expected = "Hello,\nWorld!\n";
        CHECK(expected == *promoted);
    }

    SECTION("Empty case") {
        const auto  input{R"(\\)"};
        const Token tok{TokenType::MULTILINE_STRING, input, 0, 0};

        const auto promoted = tok.promote();
        CHECK(promoted);
        const std::string expected;
        CHECK(expected == *promoted);
    }
}

TEST_CASE("Token formatting") {
    const Token tok{TokenType::STRING, R"("Hello, World!")", 1, 24};

    const auto expected{R"(STRING("Hello, World!") [1, 24])"};
    const auto actual = fmt::format("{}", tok);
    CHECK(expected == actual);
}

} // namespace porpoise::tests

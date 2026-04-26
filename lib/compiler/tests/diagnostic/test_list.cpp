#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "syntax/parser.hpp"

namespace porpoise::tests {

constexpr std::string_view input{"for (0..4) |2| { a; } else return b;"};

TEST_CASE("Dumping without source path") {
    syntax::Parser p{input};
    const auto [ast, errors] = p.consume();
    REQUIRE(ast.empty());
    std::stringstream ss;
    errors.print(ss);

    constexpr std::string_view expected{R"(ILLEGAL_IDENTIFIER 1:13
No prefix parse function for RBRACE(}) found (MISSING_PREFIX_PARSER) 1:21
)"};
    REQUIRE(ss.view() == expected);
}

TEST_CASE("Dumping with source path") {
    syntax::Parser p{input};
    const auto [ast, errors] = p.consume("sad.porp");
    REQUIRE(ast.empty());
    std::stringstream ss;
    errors.print(ss);

    constexpr std::string_view expected{R"(sad.porp:1:13: ILLEGAL_IDENTIFIER
sad.porp:1:21: No prefix parse function for RBRACE(}) found (MISSING_PREFIX_PARSER)
)"};
    REQUIRE(ss.view() == expected);
}

} // namespace porpoise::tests

#include <sstream>

#include <catch2/catch_test_macros.hpp>

#include "syntax/parser.hpp"

namespace porpoise::tests {

TEST_CASE("Dumping diagnostic lists") {
    syntax::Parser p{"for (0..4) |2| { a; } else return b;"};
    const auto [ast, errors] = p.consume();
    REQUIRE(ast.empty());
    std::stringstream ss;
    errors.print(ss);

    constexpr std::string_view expected{R"(ILLEGAL_IDENTIFIER 1:13
No prefix parse function for RBRACE(}) found (MISSING_PREFIX_PARSER) 1:21
)"};
    REQUIRE(ss.view() == expected);
}

} // namespace porpoise::tests

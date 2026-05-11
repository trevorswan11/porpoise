#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("Signed integer overflow") {
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFFF;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFFFl;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFz;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
}

TEST_CASE("Unsigned integer overflow") {
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFu;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFul;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
    helpers::test_parser_fail("0xFFFFFFFFFFFFFFFFFuz;",
                              syntax::Diagnostic{syntax::Error::INTEGER_OVERFLOW, 0, 0});
    helpers::test_parser_fail("'\\f';",
                              syntax::Diagnostic{syntax::Error::UNKNOWN_CHARACTER_ESCAPE, 0, 0});
}

TEST_CASE("Floating point overflow") {
    helpers::test_parser_fail("1023.234612e234000f;",
                              syntax::Diagnostic{syntax::Error::FLOAT_OVERFLOW, 0, 0});
    helpers::test_parser_fail("1023.234612e234000;",
                              syntax::Diagnostic{syntax::Error::DOUBLE_OVERFLOW, 0, 0});
}

} // namespace porpoise::tests

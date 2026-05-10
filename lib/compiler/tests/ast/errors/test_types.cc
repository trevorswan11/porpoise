#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"

namespace porpoise::tests {

TEST_CASE("Function type restrictions") {
    constexpr auto illegal_inputs = std::to_array<std::string_view>({
        "var a: &fn(): void;",
        "var a: &mut fn(): void;",
    });

    for (const auto& illegal : illegal_inputs) {
        helpers::test_parser_fail(
            illegal, syntax::Diagnostic{syntax::Error::ILLEGAL_FUNCTION_TYPE_MODIFIER, 0, 7});
    }
}

TEST_CASE("Bodied function type") {
    helpers::test_parser_fail("var a: *mut fn(): void { b; };",
                              syntax::Diagnostic{syntax::Error::EXPLICIT_FN_TYPE_HAS_BODY, 0, 12},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 28uz}});
}

TEST_CASE("Function return type restrictions") {
    helpers::test_parser_fail("var a: fn(): &void;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_VOID_TYPE_MODIFIER, 0, 13});
    helpers::test_parser_fail("var a: fn(): &type;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_TYPE_TYPE_MODIFIER, 0, 13});
    helpers::test_parser_fail(
        "var a: fn(): &noreturn;",
        syntax::Diagnostic{syntax::Error::ILLEGAL_NORETURN_TYPE_MODIFIER, 0, 13});
}

} // namespace porpoise::tests

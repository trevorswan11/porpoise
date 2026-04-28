#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

using MockFile = helpers::MockFile;

TEST_CASE("Duplicate identifiers") {
    helpers::test_collector_fail(
        "const a := 2; import a;",
        helpers::make_vector<MockFile>(MockFile{"a.porp", "const foo := bar;", "a"}),
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: 1:1",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Semantically illegal statements") {
    helpers::test_collector_fail("{}",
                                 sema::Diagnostic{"Cannot have block at the top level",
                                                  sema::Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                                  std::pair{0uz, 0uz}});

    helpers::test_collector_fail("defer 2;",
                                 sema::Diagnostic{"Cannot have defer outside of a function's scope",
                                                  sema::Error::ILLEGAL_TOP_LEVEL_STATEMENT,
                                                  std::pair{0uz, 0uz}});

    helpers::test_collector_fail("return 2;",
                                 sema::Diagnostic{"Cannot return outside of a function",
                                                  sema::Error::ILLEGAL_CONTROL_FLOW,
                                                  std::pair{0uz, 0uz}});

    helpers::test_collector_fail("break; continue;",
                                 sema::Diagnostic{"Cannot break outside of a loop or label",
                                                  sema::Error::ILLEGAL_CONTROL_FLOW,
                                                  std::pair{0uz, 0uz}},
                                 sema::Diagnostic{"Cannot continue outside of a loop",
                                                  sema::Error::ILLEGAL_CONTROL_FLOW,
                                                  std::pair{0uz, 7uz}});
}

using namespace std::string_view_literals;

constexpr auto RESTRICTED_INPUTS = std::array{
    std::pair{"a := fn(): void {};"sv, "function"sv},
    std::pair{"a := struct { const b := 2; };"sv, "struct"sv},
    std::pair{"a := enum { A };"sv, "enum"sv},
    std::pair{"a := union { b: bool };"sv, "union"sv},
};

TEST_CASE("Restricted non-const top level declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("var {}", input),
            sema::Diagnostic{
                fmt::format("Top level {}s must be marked const at the top level", desc),
                sema::Error::ILLEGAL_NON_CONST_STATEMENT,
                std::pair{0uz, 0uz}});
    }
}

TEST_CASE("Restricted non-const top level struct declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("const S := struct {{ var {} }};", input),
            sema::Diagnostic{
                fmt::format("Top level {}s must be marked const at the top level", desc),
                sema::Error::ILLEGAL_NON_CONST_STATEMENT,
                std::pair{0uz, 20uz}});
    }
}

TEST_CASE("Redundant constexpr usage on top level declarations") {
    for (const auto& [input, desc] : RESTRICTED_INPUTS) {
        helpers::test_collector_fail(
            fmt::format("constexpr {}", input),
            sema::Diagnostic{fmt::format("Top level {}s are implicitly compile time known", desc),
                             sema::Error::REDUNDANT_CONSTEXPR,
                             std::pair{0uz, 0uz}});
    }
}

} // namespace porpoise::tests

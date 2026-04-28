#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

TEST_CASE("Test statement symbol collection") {
    auto ctx = helpers::test_collector(R"(test "foo" { const foo := bar; })", {});
    helpers::test_hollow_symbols(ctx, helpers::TableEntry{"foo", helpers::foo_bar_decl()});
}

TEST_CASE("Test shadowing") {
    helpers::test_collector_fail(
        R"(const a := 2; test "foo" { const a := 3; })",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 27uz}});
}

TEST_CASE("Illegal test location") {
    helpers::test_collector_fail("const a := fn(&self): void { test {} };",
                                 sema::Diagnostic{"Tests must be at the topmost level of a file",
                                                  sema::Error::ILLEGAL_TEST_LOCATION,
                                                  std::pair{0uz, 29uz}});
}

} // namespace porpoise::tests

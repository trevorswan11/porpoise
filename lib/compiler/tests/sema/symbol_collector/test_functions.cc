#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("Function hollow types") {}

TEST_CASE("Well-placed function control-flow statements") {
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): void { return; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { return 0; };");
    helpers::collect_and_check("pub const main := fn(args: [][:0]u8): i32 { defer a = 2; };");
}

TEST_CASE("Constexpr function declaration") {
    helpers::collect_and_check("pub constexpr work := fn(): i32 { return 1; };");
}

TEST_CASE("Defer statements respect identifier collection rules") {
    helpers::test_collector_fail(
        "pub const main := fn(args: [][:0]u8): i32 { defer { var main: i32; } };",
        sema::Diagnostic{"Attempt to shadow identifier 'main'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 52uz}});
}

TEST_CASE("Function basic param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f: bool): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function self param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(f): void {};",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 14uz}});
}

TEST_CASE("Function local param redeclaration") {
    helpers::test_collector_fail(
        "const f := fn(a, a: bool): void {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: 1:15",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 17uz}});
}

TEST_CASE("Function block shadowing") {
    helpers::test_collector_fail(
        "const f := fn(): void { var f := 3; };",
        sema::Diagnostic{"Attempt to shadow identifier 'f'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 24uz}});
}

} // namespace porpoise::tests

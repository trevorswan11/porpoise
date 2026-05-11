#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("Do-while loop collection") {}

TEST_CASE("For loop collection") {}

TEST_CASE("Infinite loop collection") {}

TEST_CASE("While loop collection") {}

TEST_CASE("Well-placed loop control flow") {
    SECTION("For loops") {
        helpers::collect_and_check("const a := for (0..5) |i| { break; };");
        helpers::collect_and_check("const a := for (0..5) |i| { continue; };");
    }

    SECTION("Do-while loop") {
        helpers::collect_and_check("const a := do { break; } while (true);");
        helpers::collect_and_check("const a := do { continue; } while (true);");
    }

    SECTION("Infinite loop") {
        helpers::collect_and_check("const a := loop { break; };");
        helpers::collect_and_check("const a := loop { continue; };");
    }

    SECTION("While loops") {
        helpers::collect_and_check("const a := while (true) { break; };");
        helpers::collect_and_check("const a := while (true) { continue; };");
    }
}

TEST_CASE("Non-break collected as separate scope") {
    helpers::collect_and_check(
        "const a := for (0..5) |i| { const foo := bar; } else { const foo := bar; };");

    helpers::collect_and_check(
        "const a := while (true) : (i += 1) { const foo := bar; } else { const foo := bar; };");
}

TEST_CASE("Non-break collection shadowing") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 55uz}});

    helpers::test_collector_fail(
        "const a := while (true) : (i += 1) { const foo := bar; } else { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 64uz}});
}

TEST_CASE("Shadowing in loops") {
    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 28uz}});

    helpers::test_collector_fail(
        "const a := for (0..5) |i| { var i: i32; };",
        sema::Diagnostic{"Redeclaration of symbol 'i'. Previous declaration here: 1:24",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{0uz, 28uz}});

    helpers::test_collector_fail(
        "const a := loop { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 18uz}});

    helpers::test_collector_fail(
        "const a := while (true) { var a: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 26uz}});
}

} // namespace porpoise::tests

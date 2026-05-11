#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("Struct hollow types") {}

TEST_CASE("Enum hollow types") {}

TEST_CASE("Enum hollow types with member") {}

TEST_CASE("Union hollow types") {}

TEST_CASE("Union hollow types with member") {}

TEST_CASE("Public using query") {}

TEST_CASE("Shadowing member/field declarations") {
    helpers::test_collector_fail(
        "const a := struct { var a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 20uz}});

    helpers::test_collector_fail(
        "const a := enum {a};",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 17uz}});

    helpers::test_collector_fail(
        "const a := enum {b static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 19uz}});

    helpers::test_collector_fail(
        "const a := union { a: i32 };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 19uz}});

    helpers::test_collector_fail(
        "const a := union { b: i32 static const a := 2; };",
        sema::Diagnostic{"Attempt to shadow identifier 'a'. Previous declaration here: 1:1",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{0uz, 26uz}});
}

} // namespace porpoise::tests

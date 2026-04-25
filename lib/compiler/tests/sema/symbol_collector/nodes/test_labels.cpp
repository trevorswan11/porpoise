#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

namespace helpers {

auto collect_and_validate_label(std::string_view input, usize expected_size) -> void {
    auto [analyzer, idx] = helpers::collect_and_validate(input);

    const auto& registry = analyzer.get_registry();
    REQUIRE(registry.size() == expected_size);

    const auto& table = registry.get(idx);
    CHECK(table.has("a"));
    CHECK(table.has("blk"));
}

} // namespace helpers

TEST_CASE("Label collection") {
    helpers::collect_and_validate_label("const a := blk: for (0..5) |i| { const foo := bar; };", 2);
    helpers::collect_and_validate_label(
        "const a := blk: { if (b) { break :blk c; } else break :blk 5; };", 3);
}

TEST_CASE("Label redeclaration") {
    helpers::test_collector_fail(
        "const a := a: {};",
        sema::Diagnostic{"Redeclaration of symbol 'a'. Previous declaration here: 1:1",
                         sema::Error::IDENTIFIER_REDECLARATION,
                         std::pair{1uz, 12uz}});
}

TEST_CASE("Label shadowing") {
    helpers::test_collector_fail(
        "const a := blk: { var blk: i32; };",
        sema::Diagnostic{"Attempt to shadow identifier 'blk'. Previous declaration here: 1:12",
                         sema::Error::SHADOWING_DECLARATION,
                         std::pair{1uz, 19uz}});
}

} // namespace porpoise::tests

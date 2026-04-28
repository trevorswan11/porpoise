#include <ranges>

#include <catch2/catch_test_macros.hpp>

#include "diagnostic/source_file.hpp"

namespace porpoise::tests {

// clang-format off
constexpr std::string_view source{
    R"(This is line 1
This is line 2
    This is line 3 that starts with spaces
Line 4 has      some weird   spacing...
    Line 5 is    unrealistic.. .
)"};
// clang-format on

namespace helpers {

auto test_diag_strings(const SourceLocation&         t,
                       std::string_view              expected_line,
                       opt::Option<std::string_view> expected_caret) {
    const SourceFile file{source};
    const auto [ln, caret] = file.get_diagnostic_strings(t);

    CHECK(ln == expected_line);
    if (expected_caret) {
        REQUIRE(caret);
        CHECK(*caret == *expected_caret);
    } else {
        CHECK_FALSE(caret);
    }
}

} // namespace helpers

TEST_CASE("Offset generation") {
    LineOffsets offsets{source};
    CHECK(offsets.size() == 6);

    constexpr std::array expected_mappings{0uz, 15uz, 30uz, 73uz, 113uz, 146uz};
    for (const auto& [offset, expected] : std::views::zip(offsets, expected_mappings)) {
        CHECK(offset == expected);
    }
}

TEST_CASE("Diagnostic at start of input") {
    helpers::test_diag_strings({0uz, 0uz}, "This is line 1", "^");
}

TEST_CASE("Diagnostic inside of first line") {
    helpers::test_diag_strings({0uz, 1uz}, "This is line 1", " ^");
    helpers::test_diag_strings({0uz, 5uz}, "This is line 1", "     ^");
}

} // namespace porpoise::tests

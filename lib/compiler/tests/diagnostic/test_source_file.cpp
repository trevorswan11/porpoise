#include <ranges>

#include <catch2/catch_test_macros.hpp>

#include "diagnostic/source_file.hpp"

namespace porpoise::tests {

constexpr std::string_view source{
    R"(This is line 1
This is line 2
    This is line 3 that starts with spaces
Line 4 has      some weird   spacing...
    Line 5 is    unrealistic.. .
)"};

TEST_CASE("Offset generation") {
    LineOffsets offsets{source};
    CHECK(offsets.size() == 6);

    constexpr std::array expected_mappings{0uz, 15uz, 30uz, 73uz, 113uz, 146uz};
    for (const auto& [offset, expected] : std::views::zip(offsets, expected_mappings)) {
        CHECK(offset == expected);
    }
}

using namespace std::string_view_literals;

TEST_CASE("Line 1 Column 0 Diagnostic") {
    SourceFile file{source};
    const auto [ln, caret] = file.get_diagnostic_strings({1uz, 0uz});

    CHECK(ln == "This is line 1");
    REQUIRE(caret);
    CHECK(*caret == "^");
}

} // namespace porpoise::tests

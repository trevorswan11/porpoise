#include <catch2/catch_test_macros.hpp>

#include "string.hpp"

namespace porpoise::tests {

TEST_CASE("Byte type requirement") {
    STATIC_CHECK(std::is_same_v<std::string::value_type, byte>);
    STATIC_CHECK(std::is_same_v<char, byte>);
}

TEST_CASE("Is space") {
    CHECK(string::is_space(' '));
    CHECK(string::is_space('\t'));
    CHECK(string::is_space('\n'));
    CHECK(string::is_space('\r'));
    CHECK_FALSE(string::is_space('\\'));
}

TEST_CASE("Left trim spaces") {
    usize count = 0;
    CHECK(string::trim_left("", count) == "");
    CHECK(count == 0);

    CHECK(string::trim_left("the", (count = 0, count)) == "the");
    CHECK(count == 0);

    CHECK(string::trim_left("    the", (count = 0, count)) == "the");
    CHECK(count == 4);

    CHECK(string::trim_left("        ", (count = 0, count)) == "");
    CHECK(count == 8);
}

TEST_CASE("Right trim spaces") {
    usize count = 0;
    CHECK(string::trim_right("", count) == "");
    CHECK(count == 0);

    CHECK(string::trim_right("the", (count = 0, count)) == "the");
    CHECK(count == 0);

    CHECK(string::trim_right("the    ", (count = 0, count)) == "the");
    CHECK(count == 4);

    CHECK(string::trim_right("        ", (count = 0, count)) == "");
    CHECK(count == 8);
}

TEST_CASE("Trim spaces") {
    usize count = 0;
    CHECK(string::trim("", count) == "");
    CHECK(count == 0);

    CHECK(string::trim("the", (count = 0, count)) == "the");
    CHECK(count == 0);

    CHECK(string::trim("the    ", (count = 0, count)) == "the");
    CHECK(count == 4);

    CHECK(string::trim("    the", (count = 0, count)) == "the");
    CHECK(count == 4);

    CHECK(string::trim("    the    ", (count = 0, count)) == "the");
    CHECK(count == 8);

    CHECK(string::trim("        ", (count = 0, count)) == "");
    CHECK(count == 8);
}

TEST_CASE("Trim pred") {
    CHECK(string::trim("theasdaefae",
                       [](byte b) { return std::string_view{"asdaefae"}.contains(b); }) == "th");
}

TEST_CASE("String view substrings") {
    constexpr std::string_view str{"abcdefghijk"};
    CHECK(string::substr(str, 2) == "cdefghijk");
    CHECK(string::substr(str, 2, 7) == "cdefghi");
    CHECK(string::substr(str, 100) == "");
}

} // namespace porpoise::tests

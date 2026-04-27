#include <string>

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
    CHECK(string::trim_left("") == "");
    CHECK(string::trim_left("the") == "the");
    CHECK(string::trim_left("    the") == "the");
    CHECK(string::trim_left("    the    ") == "the    ");
    CHECK(string::trim_left("        ") == "");
}

TEST_CASE("Counting left trim") {
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

TEST_CASE("Counting right trim") {
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
    CHECK(string::trim("") == "");
    CHECK(string::trim("the") == "the");
    CHECK(string::trim("the    ") == "the");
    CHECK(string::trim("    the") == "the");
    CHECK(string::trim("    the    ") == "the");
    CHECK(string::trim("        ") == "");
}

TEST_CASE("Counting trim") {
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

} // namespace porpoise::tests

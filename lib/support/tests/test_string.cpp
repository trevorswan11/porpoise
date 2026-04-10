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

TEST_CASE("Right trim spaces") {
    CHECK(string::trim_right("") == "");
    CHECK(string::trim_right("the") == "the");
    CHECK(string::trim_right("the    ") == "the");
    CHECK(string::trim_right("    the    ") == "    the");
    CHECK(string::trim_right("        ") == "");
}

TEST_CASE("Trim spaces") {
    CHECK(string::trim("") == "");
    CHECK(string::trim("the") == "the");
    CHECK(string::trim("the    ") == "the");
    CHECK(string::trim("    the") == "the");
    CHECK(string::trim("    the    ") == "the");
    CHECK(string::trim("        ") == "");
}

TEST_CASE("Trim pred") {
    CHECK(string::trim("theasdaefae",
                       [](byte b) { return std::string_view{"asdaefae"}.contains(b); }) == "th");
}

} // namespace porpoise::tests

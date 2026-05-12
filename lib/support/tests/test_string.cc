#include <catch2/catch_test_macros.hpp>

#include "string.hh"

namespace porpoise::tests {

TEST_CASE("String traits") {
    STATIC_CHECK(traits::StringLike<std::string>);
    STATIC_CHECK(traits::StringLike<std::string_view>);
    STATIC_CHECK(traits::StringLike<const byte*>);
    STATIC_CHECK_FALSE(traits::StdStringLike<const byte*>);
}

TEST_CASE("Byte type requirement") {
    STATIC_CHECK(std::is_same_v<std::string::value_type, byte>);
    STATIC_CHECK(std::is_same_v<char, byte>);
}

TEST_CASE("Left trim spaces") {
    CHECK(string::trim_left("") == "");
    CHECK(string::trim_left("the") == "the");
    CHECK(string::trim_left("    the") == "the");
    CHECK(string::trim_left("        ") == "");
}

TEST_CASE("Right trim spaces") {
    CHECK(string::trim_right("") == "");
    CHECK(string::trim_right("the") == "the");
    CHECK(string::trim_right("the    ") == "the");
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

TEST_CASE("String view substrings") {
    constexpr std::string_view str{"abcdefghijk"};
    CHECK(string::substr(str, 2) == "cdefghijk");
    CHECK(string::substr(str, 2, 7) == "cdefghi");
    CHECK(string::substr(str, 100) == "");
}

TEST_CASE("Blank string check") {
    CHECK_FALSE(string::is_blank("const T &text"));
    CHECK(string::is_blank("        "));
    CHECK(string::is_blank("        \t\n\r"));
}

} // namespace porpoise::tests

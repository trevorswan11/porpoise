#include <string>

#include <catch2/catch_test_macros.hpp>

#include "module/error.hh"
#include "module/memory_loader.hh"

namespace porpoise::tests {

TEST_CASE("Correct add/load cycle") {
    const std::string expected_content = "This is a file";
    mod::MemoryLoader loader;
    loader.add("mock", expected_content);

    const auto content = loader.load("mock");
    REQUIRE(content);
    CHECK(*content == expected_content);
}

TEST_CASE("Overwriting entries in the VFS") {
    mod::MemoryLoader loader;
    loader.add("mock", "This is the bad content");

    const std::string expected_content = "This is the good content";
    loader.add("mock", expected_content);

    const auto content = loader.load("mock");
    REQUIRE(content);
    CHECK(*content == expected_content);
}

TEST_CASE("Missing path in VFS") {
    mod::MemoryLoader loader;
    const auto        result = loader.load("mock");
    REQUIRE_FALSE(result);

    const mod::Diagnostic expected{"Could not find path 'mock' in virtual file system",
                                   mod::Error::PATH_DOES_NOT_EXIST};
    CHECK(result.error() == expected);
}

} // namespace porpoise::tests

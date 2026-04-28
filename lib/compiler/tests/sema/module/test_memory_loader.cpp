#include <catch2/catch_test_macros.hpp>

#include "sema/module/memory_loader.hpp"

namespace porpoise::tests {

TEST_CASE("Correct add/load cycle") {
    const std::string       expected_content = "This is a file";
    sema::mod::MemoryLoader loader;
    loader.add("mock", expected_content);

    const auto content = loader.load("mock");
    REQUIRE(content);
    CHECK(*content == expected_content);
}

TEST_CASE("Overwriting entries in the VFS") {
    sema::mod::MemoryLoader loader;
    loader.add("mock", "This is the bad content");

    const std::string expected_content = "This is the good content";
    loader.add("mock", expected_content);

    const auto content = loader.load("mock");
    REQUIRE(content);
    CHECK(*content == expected_content);
}

TEST_CASE("Missing path in VFS") {
    sema::mod::MemoryLoader loader;
    const auto              result = loader.load("mock");
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{"Could not find path 'mock' in virtual file system",
                                    sema::Error::PATH_DOES_NOT_EXIST};
    CHECK(result.error() == expected);
}

} // namespace porpoise::tests

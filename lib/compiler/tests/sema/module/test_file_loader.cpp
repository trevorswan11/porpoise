#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "sema/module/file_loader.hpp"

namespace porpoise::tests {

constexpr std::string_view expected_contents{"This is porpoise-ful\n"};

TEST_CASE("Reading file off of disk correctly") {
    sema::mod::FileLoader loader;
    const auto            normalized = loader.normalize("lib/compiler/tests/sema/module/mock.inc");
    REQUIRE(normalized);

    const auto contents = loader.load(*normalized);
    REQUIRE(contents);
    CHECK(contents->size() == expected_contents.size());
    CHECK(*contents == expected_contents);
}

TEST_CASE("Path load on file not on disk") {
    sema::mod::FileLoader loader;
    const auto            normalized = loader.normalize("lib/compiler/tests/sema/module/mock");
    REQUIRE(normalized);

    const auto result = loader.load(*normalized);
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{fmt::format("Path '{}' does not exist", normalized->string()),
                                    sema::Error::PATH_DOES_NOT_EXIST};
    CHECK(result.error() == expected);
}

TEST_CASE("Path load on directory on disk") {
    sema::mod::FileLoader loader;
    const auto            normalized = loader.normalize("lib/compiler/tests/sema/module");
    REQUIRE(normalized);

    const auto result = loader.load(*normalized);
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{fmt::format("Path '{}' is not a file", normalized->string()),
                                    sema::Error::PATH_IS_NOT_FILE};
    CHECK(result.error() == expected);
}

} // namespace porpoise::tests

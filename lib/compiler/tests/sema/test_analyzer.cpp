#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hpp"

namespace porpoise::tests {

using MockFile = helpers::MockFile;

constexpr std::string_view main_porp{R"(
import std;

pub const main := fn(args: [][:0]u8): i32 {
    const message := "Hello, world!";
    std::io::println(message);
};
)"};

constexpr std::string_view std_porp{R"(
pub import "io.porp" as io;
)"};

constexpr std::string_view io_porp{R"(
pub const println := fn(str: []u8): void {};
)"};

TEST_CASE("Full sema pipeline") {
    constexpr std::string_view root{"main.porp"};

    auto ctx = helpers::analyze(
        root, main_porp, MockFile{"std.porp", std_porp, "std"}, MockFile{"io.porp", io_porp});
    const auto& registry = ctx.analyzer.get_registry();
    REQUIRE(registry.size() == 5);

    CHECK(registry.get_from_opt(0, "std"));
    CHECK(registry.get_from_opt(0, "main"));

    // Imports are eager, so std and io get evaluated before main
    CHECK(registry.get_from_opt(1, "io"));
    CHECK(registry.get_from_opt(2, "println"));
    CHECK(registry.get_from_opt(3, "str"));

    CHECK(registry.get_from_opt(4, "args"));
    CHECK(registry.get_from_opt(4, "message"));
}

} // namespace porpoise::tests

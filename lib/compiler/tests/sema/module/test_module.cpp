#include <catch2/catch_test_macros.hpp>

#include "sema/module/memory_loader.hpp"
#include "sema/module/module.hpp"

#include <config.h>

namespace porpoise::tests {

TEST_CASE("Fetching non-relative file modules") {
    sema::mod::MemoryLoader  loader;
    sema::mod::ModuleManager manager{loader};

#if PLATFORM_WINDOWS
    const std::string_view file{"C:/fake/foo.porp"};
#else
    const std::string_view file{"/fake/foo.porp"};
#endif
    const auto result = manager.try_get_file_module(file);
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{
        fmt::format("Requested file '{}' is absolute", file),
        sema::Error::MODULE_PATH_NOT_RELATIVE,
    };
    CHECK(result.error() == expected);
}

TEST_CASE("Fetching missing library modules") {
    sema::mod::MemoryLoader  loader;
    sema::mod::ModuleManager manager{loader};

    const auto result = manager.try_get_library_module("foo");
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{
        "Unknown module 'foo'",
        sema::Error::MODULE_DOES_NOT_EXIST,
    };
    CHECK(result.error() == expected);
}

TEST_CASE("Adding duplicate library module") {
    sema::mod::MemoryLoader  loader;
    sema::mod::ModuleManager manager{loader};
    REQUIRE(manager.add_library_module("foo", "foo.porp"));

    const auto result = manager.add_library_module("foo", "src/foo.porp");
    REQUIRE_FALSE(result);

    const sema::Diagnostic expected{
        "Attempt to add duplicate module 'foo' from path 'src/foo.porp' which already exists at "
        "path 'foo.porp'",
        sema::Error::MODULE_ALREADY_EXISTS,
    };
    CHECK(result.error() == expected);
}

} // namespace porpoise::tests

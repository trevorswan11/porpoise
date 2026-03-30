#include <catch2/catch_test_macros.hpp>

#include "memory.hpp"

namespace porpoise::tests {

using OkBox     = mem::Box<bool>;
using NotBox    = bool;
using CustomBox = mem::Box<bool, void (*)(bool*)>;

TEST_CASE("Box template checks") {
    STATIC_CHECK(mem::is_box<std::unique_ptr<bool>>::value);
    STATIC_CHECK(mem::is_box_v<OkBox>);
    STATIC_CHECK_FALSE(mem::is_box_v<NotBox>);
    STATIC_CHECK(mem::is_box_v<CustomBox>);
}

} // namespace porpoise::tests

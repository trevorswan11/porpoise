#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace porpoise::tests {

TEST_CASE("Array resolution") { helpers::resolve_and_check("const a := [2]i32{1, 2, };"); }

} // namespace porpoise::tests

#pragma once

#include "expected.hpp"
#include "types.hpp"
#include "variant.hpp"

namespace porpoise::driver {

// Parses command line arguments and dispatches the input
auto launch(i32 argc, byte* argv[]) -> Expected<unit, i32>;

} // namespace porpoise::driver

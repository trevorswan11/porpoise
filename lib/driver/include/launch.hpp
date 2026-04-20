#pragma once

#include "result.hpp"
#include "types.hpp"
#include "variant.hpp"

namespace porpoise::driver {

// Parses command line arguments and dispatches the input
auto launch(i32 argc, byte** argv) -> Result<Unit, i32>;

} // namespace porpoise::driver

#pragma once

#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::driver {

// Parses command line arguments and dispatches the input
auto launch(i32 argc, byte** argv) -> Result<Unit, i32>;

} // namespace porpoise::driver

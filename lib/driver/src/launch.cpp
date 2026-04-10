#include "launch.hpp"

#include "arguments/parser.hpp"

namespace porpoise::driver {

auto launch(i32 argc, byte* argv[]) -> Expected<unit, i32> {
    driver::Parser parser{argc, argv};
    TRY(parser.parse());
    return parser.dispatch();
}

} // namespace porpoise::driver

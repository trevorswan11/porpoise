#include "launch.hh"

#include "arguments/parser.hh"

namespace porpoise::driver {

auto launch(i32 argc, byte** argv) -> Result<Unit, i32> {
    driver::Parser parser{argc, argv};
    TRY(parser.parse());
    return parser.dispatch();
}

} // namespace porpoise::driver

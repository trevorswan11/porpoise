#include "launch.hh"

#include <variant>

#include "clap/parser.hh"
#include "cmd/dispatcher.hh"

#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::driver {

auto launch(i32 argc, byte** argv) -> Result<Unit, i32> {
    clap::Parser parser{argc, argv};
    TRY(parser.parse());

    cmd::Dispatcher dispatcher;
    TRY(std::visit(dispatcher, parser.get_parsed()));
    return Unit{};
}

} // namespace porpoise::driver

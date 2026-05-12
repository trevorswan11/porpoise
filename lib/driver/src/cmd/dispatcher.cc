#include "cmd/dispatcher.hh"

#include "cmd/debug.hh"

namespace porpoise::cmd {

auto Dispatcher::operator()(Debug& dump) -> Result<Unit, i32> {
    dump.run();
    return Unit{};
}

} // namespace porpoise::cmd

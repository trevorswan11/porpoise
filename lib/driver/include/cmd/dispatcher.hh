#pragma once

#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::cmd {

class Debug;

class Dispatcher {
  public:
    static auto operator()(Debug& dump) -> Result<Unit, i32>;
};

} // namespace porpoise::cmd

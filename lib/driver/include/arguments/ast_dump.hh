#pragma once

#include <string>

#include "syntax/parser.hh"

namespace porpoise::driver {

class AstDump {
  public:
    auto run() -> void;

  private:
    syntax::Parser parser_;
    std::string    line_;
};

} // namespace porpoise::driver

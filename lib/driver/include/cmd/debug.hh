#pragma once

#include <string>

namespace porpoise::cmd {

class Debug {
  public:
    auto run() -> void;

  private:
    std::string line_;
};

} // namespace porpoise::cmd

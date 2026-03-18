#pragma once

#include "parser/parser.hpp"

#ifdef _WIN32
#    include "platform/win32.hpp"
#endif

namespace porpoise::cli {

class Program {
  public:
    Program() noexcept = default;

    auto interactive() -> void;

  private:
    Parser parser_;
#ifdef _WIN32
    win32::RichConsole console_;
#endif
};

} // namespace porpoise::cli

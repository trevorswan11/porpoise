#pragma once

#include "syntax/parser.hpp"

#ifdef _WIN32
#    include "platform/win32.hpp"
#endif

namespace porpoise {

class Program {
  public:
    Program() noexcept = default;

    auto interactive() -> void;

  private:
    syntax::Parser parser_;
#ifdef _WIN32
    win32::RichConsole console_;
#endif
};

} // namespace porpoise

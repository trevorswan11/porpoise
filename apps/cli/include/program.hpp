#pragma once

#include "parser/parser.hpp"

#include "platform/win32.hpp"

namespace conch::cli {

class Program {
  public:
    Program() noexcept = default;

    auto interactive() -> void;

  private:
    Parser             parser_;
    win32::RichConsole console_;
};

} // namespace conch::cli

#pragma once

#include <iostream>
#include <ostream>

#include <CLI/App.hpp>

#include "cmd/debug.hh"

#include <config.h>

#if PLATFORM_WINDOWS
#    include "platform/win32.hh"
#endif

#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::clap {

using Parsed = std::variant<Unit, cmd::Debug>;

class Parser {
  public:
    Parser(i32 argc, byte** argv, std::ostream& os = std::cerr) noexcept;

    auto               parse() -> Result<Unit, i32>;
    [[nodiscard]] auto get_parsed() noexcept -> Parsed& { return parsed_; }

  private:
    i32           argc_;
    byte**        argv_;
    std::ostream& os_;

    CLI::App app_;
    Parsed   parsed_;

#if PLATFORM_WINDOWS
    win32::RichConsole console_;
#endif
};

} // namespace porpoise::clap

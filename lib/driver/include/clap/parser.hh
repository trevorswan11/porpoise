#pragma once

#include <CLI/App.hpp>

#include <config.h>

#include "cmd/debug.hh"

#if PLATFORM_WINDOWS
#    include "platform/win32.hh"
#endif

#include "result.hh"
#include "types.hh"
#include "variant.hh"

namespace porpoise::clap {

using Parsed = std::variant<cmd::Debug>;

class Parser {
  public:
    Parser(i32 argc, byte** argv) noexcept;

    auto               parse() -> Result<Unit, i32>;
    [[nodiscard]] auto get_parsed() noexcept -> Parsed& { return parsed_; }

  private:
    i32    argc_;
    byte** argv_;

    CLI::App app_;
    Parsed   parsed_;

#if PLATFORM_WINDOWS
    win32::RichConsole console_;
#endif
};

} // namespace porpoise::clap

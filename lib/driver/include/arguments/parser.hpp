#pragma once

#include <config.h>

#include "arguments/ast_dump.hpp"

#if PLATFORM_WINDOWS
#    include "platform/win32.hpp"
#endif

#include "result.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise::driver {

using Parsed = std::variant<AstDump>;

class Parser {
  public:
    Parser(i32 argc, byte** argv) noexcept : argc_{argc}, argv_{argv} {}

    auto parse() -> Result<Unit, i32>;
    auto dispatch() -> Unit;

    MAKE_GETTER(parsed, Parsed)

  private:
    i32    argc_;
    byte** argv_;

    Parsed parsed_;

#if PLATFORM_WINDOWS
    driver::win32::RichConsole console_;
#endif
};

} // namespace porpoise::driver

#pragma once

#include "arguments/ast_dump.hpp"

#ifdef _WIN32
#    include "platform/win32.hpp"
#endif

#include "expected.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "variant.hpp"

namespace porpoise::driver {

using Parsed = std::variant<AstDump>;

class Parser {
  public:
    Parser(i32 argc, byte** argv) noexcept : argc_{argc}, argv_{argv} {}

    auto parse() -> Expected<unit, i32>;
    auto dispatch() -> unit;

    MAKE_GETTER(parsed, Parsed)

  private:
    i32    argc_;
    byte** argv_;

    Parsed parsed_;

#ifdef _WIN32
    driver::win32::RichConsole console_;
#endif
};

} // namespace porpoise::driver

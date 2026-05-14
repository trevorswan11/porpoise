#include <iostream>

#include <fmt/color.h>
#include <fmt/ostream.h>

#include <CLI/CLI.hpp>

#include "clap/formatter.hh"
#include "clap/parser.hh"

#include "memory.hh"
#include "style.hh"

#include <config.h>

namespace porpoise::clap {

Parser::Parser(i32 argc, byte** argv) noexcept : argc_{argc} {
    app_.formatter(mem::make_rc<CLIFmt>());
    argv_ = app_.ensure_utf8(argv);
}

auto Parser::parse() -> Result<Unit, i32> {
    app_.usage("Usage: porpoise [command] [options]");
    app_.set_version_flag("-v,--version", fmt::format("porpoise v{} ({})", VERSION_STR, GIT_INFO));
    app_.require_subcommand(1);

    const auto* ast_app = app_.add_subcommand("debug", "Run the CLI interactive debugger");

    // No arguments should be handled by printing help an exiting
    if (argc_ == 1) {
        fmt::println(std::cerr, "{}", app_.help());
        fmt::print(style::RED_BOLD, "error");
        fmt::println(std::cerr, ": expected command argument");
        return Err{1};
    }

    try {
        app_.parse(argc_, argv_);
    } catch (const CLI::ParseError& e) { return Err{app_.exit(e)}; };
    if (ast_app->parsed()) { parsed_.emplace<cmd::Debug>(); }

    return Unit{};
}

} // namespace porpoise::clap

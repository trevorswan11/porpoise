#include <fmt/format.h>

#include <CLI/CLI.hpp>

#include "arguments/parser.hpp"

#include <config.h>

namespace porpoise::driver {

auto Parser::parse() -> Expected<Unit, i32> {
    CLI::App app{fmt::format("porpoise language compiler v{}", VERSION_STR)};
    app.set_version_flag("-v,--version", fmt::format("porpoise v{} ({})", VERSION_STR, GIT_INFO));

    app.require_subcommand(1);
    const auto* argv = app.ensure_utf8(argv_);

    const auto* ast_app = app.add_subcommand("ast", "Run the ast visualizer");

    try {
        app.parse(static_cast<i32>(argc_), argv);
    } catch (const CLI::ParseError& e) { return Unexpected{app.exit(e)}; };
    if (ast_app->parsed()) { parsed_.emplace<AstDump>(); }

    return Unit{};
}

auto Parser::dispatch() -> Unit {
    std::visit(Overloaded{[](AstDump& d) { d.run(); }}, parsed_);
    return {};
}

} // namespace porpoise::driver

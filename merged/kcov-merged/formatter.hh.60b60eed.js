var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include <CLI/Formatter.hpp>"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::clap {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"class CLIFmt : public CLI::Formatter {","class":"lineCov","hits":"1","order":"3636",},
{"lineNum":"    8","line":"  public:"},
{"lineNum":"    9","line":"    CLIFmt() noexcept = default;","class":"lineCov","hits":"1","order":"3635",},
{"lineNum":"   10","line":""},
{"lineNum":"   11","line":"    [[nodiscard]] auto make_subcommands(const CLI::App* app, CLI::AppFormatMode) const"},
{"lineNum":"   12","line":"        -> std::string override;"},
{"lineNum":"   13","line":""},
{"lineNum":"   14","line":"    [[nodiscard]] auto"},
{"lineNum":"   15","line":"    make_group(std::string group, bool is_positional, std::vector<const CLI::Option*> opts) const"},
{"lineNum":"   16","line":"        -> std::string override;"},
{"lineNum":"   17","line":""},
{"lineNum":"   18","line":"    [[nodiscard]] auto make_help(const CLI::App*    app,"},
{"lineNum":"   19","line":"                                 std::string        name,"},
{"lineNum":"   20","line":"                                 CLI::AppFormatMode mode) const -> std::string override;"},
{"lineNum":"   21","line":"};"},
{"lineNum":"   22","line":""},
{"lineNum":"   23","line":"} // namespace porpoise::clap"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-05-14 22:03:35", "instrumented" : 2, "covered" : 2,};
var merged_data = [];

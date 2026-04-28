var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include <filesystem>"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"#include \"sema/error.hpp\""},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"#include \"result.hpp\""},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"namespace porpoise::sema::mod {"},
{"lineNum":"   10","line":""},
{"lineNum":"   11","line":"class SourceLoader {","class":"lineCov","hits":"1","order":"386",},
{"lineNum":"   12","line":"  public:"},
{"lineNum":"   13","line":"    virtual ~SourceLoader() = default;","class":"lineCov","hits":"1","order":"387",},
{"lineNum":"   14","line":"    [[nodiscard]] virtual auto load(const std::filesystem::path& path)"},
{"lineNum":"   15","line":"        -> Result<std::string, Error> = 0;"},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"    // Normalizes the path to behave as required by the loader"},
{"lineNum":"   18","line":"    [[nodiscard]] virtual auto normalize(const std::filesystem::path& path)"},
{"lineNum":"   19","line":"        -> Result<std::filesystem::path, Error> = 0;"},
{"lineNum":"   20","line":"};"},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace porpoise::sema::mod"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 2, "covered" : 2,};
var merged_data = [];

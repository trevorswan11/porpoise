var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"source_loader.hh\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::mod {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"class FileLoader : public SourceLoader {","class":"lineCov","hits":"1","order":"2370",},
{"lineNum":"    8","line":"  public:"},
{"lineNum":"    9","line":"    // Attempts to obtain the file\'s source code from disk and load it into memory"},
{"lineNum":"   10","line":"    [[nodiscard]] auto load(const std::filesystem::path& path)"},
{"lineNum":"   11","line":"        -> Result<std::string, Diagnostic> override;"},
{"lineNum":"   12","line":""},
{"lineNum":"   13","line":"    // Converts the path to its weakly canonical representation"},
{"lineNum":"   14","line":"    [[nodiscard]] auto normalize(const std::filesystem::path& path)"},
{"lineNum":"   15","line":"        -> Result<std::filesystem::path, Error> override;"},
{"lineNum":"   16","line":"};"},
{"lineNum":"   17","line":""},
{"lineNum":"   18","line":"} // namespace porpoise::mod"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-05-14 22:03:35", "instrumented" : 1, "covered" : 1,};
var merged_data = [];

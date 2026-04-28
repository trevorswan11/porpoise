var data = {lines:[
{"lineNum":"    1","line":"#include \"sema/module/memory_loader.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"namespace porpoise::sema::mod {"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"auto MemoryLoader::add(std::filesystem::path path, const std::string& content) -> void {","class":"lineCov","hits":"1","order":"3127",},
{"lineNum":"    6","line":"    auto normalized = normalize(path);","class":"lineCov","hits":"1","order":"3126",},
{"lineNum":"    7","line":"    assert(normalized);","class":"lineCov","hits":"1","order":"3124",},
{"lineNum":"    8","line":"    files_[*normalized] = std::move(content);","class":"lineCov","hits":"1","order":"3123",},
{"lineNum":"    9","line":"}","class":"lineCov","hits":"1","order":"3122",},
{"lineNum":"   10","line":""},
{"lineNum":"   11","line":"auto MemoryLoader::load(const std::filesystem::path& path) -> Result<std::string, Error> {","class":"lineCov","hits":"1","order":"3121",},
{"lineNum":"   12","line":"    auto normalized = normalize(path);","class":"lineCov","hits":"1","order":"3120",},
{"lineNum":"   13","line":"    assert(normalized);","class":"lineCov","hits":"1","order":"3119",},
{"lineNum":"   14","line":"    auto it = files_.find(normalized->string());","class":"lineCov","hits":"1","order":"3118",},
{"lineNum":"   15","line":"    if (it == files_.end()) { return Err{Error::PATH_DOES_NOT_EXIST}; }","class":"lineCov","hits":"1","order":"3117",},
{"lineNum":"   16","line":"    return it->second;","class":"lineCov","hits":"1","order":"3116",},
{"lineNum":"   17","line":"}","class":"lineCov","hits":"1","order":"3125",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace porpoise::sema::mod"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 12, "covered" : 12,};
var merged_data = [];

var data = {lines:[
{"lineNum":"    1","line":"#include \"module/memory_loader.hh\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"namespace porpoise::mod {"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"auto MemoryLoader::add(const std::filesystem::path& path, const std::string& content) -> void {","class":"lineCov","hits":"1","order":"3180",},
{"lineNum":"    6","line":"    const auto normalized = normalize(path);","class":"lineCov","hits":"1","order":"3179",},
{"lineNum":"    7","line":"    ASSERT(normalized);","class":"lineCov","hits":"1","order":"3177",},
{"lineNum":"    8","line":"    files_[*normalized] = std::move(content);","class":"lineCov","hits":"1","order":"3176",},
{"lineNum":"    9","line":"}","class":"lineCov","hits":"1","order":"3175",},
{"lineNum":"   10","line":""},
{"lineNum":"   11","line":"auto MemoryLoader::load(const std::filesystem::path& path) -> Result<std::string, Diagnostic> {","class":"lineCov","hits":"1","order":"3174",},
{"lineNum":"   12","line":"    auto normalized = normalize(path);","class":"lineCov","hits":"1","order":"3173",},
{"lineNum":"   13","line":"    ASSERT(normalized);","class":"lineCov","hits":"1","order":"3172",},
{"lineNum":"   14","line":"    auto it = files_.find(normalized->string());","class":"lineCov","hits":"1","order":"3171",},
{"lineNum":"   15","line":"    if (it == files_.end()) {","class":"lineCov","hits":"1","order":"3170",},
{"lineNum":"   16","line":"        return make_mod_err(","class":"lineCov","hits":"1","order":"3168",},
{"lineNum":"   17","line":"            fmt::format(\"Could not find path \'{}\' in virtual file system\", path.string()),","class":"lineCov","hits":"1","order":"3178",},
{"lineNum":"   18","line":"            Error::PATH_DOES_NOT_EXIST);","class":"lineCov","hits":"1","order":"3169",},
{"lineNum":"   19","line":"    }"},
{"lineNum":"   20","line":"    return it->second;","class":"lineCov","hits":"1","order":"3166",},
{"lineNum":"   21","line":"}","class":"lineCov","hits":"1","order":"3167",},
{"lineNum":"   22","line":""},
{"lineNum":"   23","line":"} // namespace porpoise::mod"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-05-14 22:03:35", "instrumented" : 15, "covered" : 15,};
var merged_data = [];

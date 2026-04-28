var data = {lines:[
{"lineNum":"    1","line":"#include \"sema/analyzer.hpp\""},
{"lineNum":"    2","line":"#include \"sema/error.hpp\""},
{"lineNum":"    3","line":"#include \"sema/symbol_collector.hpp\""},
{"lineNum":"    4","line":"#include \"sema/type_resolver.hpp\""},
{"lineNum":"    5","line":""},
{"lineNum":"    6","line":"namespace porpoise::sema {"},
{"lineNum":"    7","line":""},
{"lineNum":"    8","line":"auto Analyzer::analyze(const std::filesystem::path& entry_path) -> Result<Unit, Diagnostic> {","class":"lineCov","hits":"1","order":"2836",},
{"lineNum":"    9","line":"    auto module = TRY(modules_.try_get_file_module(entry_path));","class":"lineCov","hits":"1","order":"2835",},
{"lineNum":"   10","line":"    collect_symbols(*module);","class":"lineCov","hits":"1","order":"2833",},
{"lineNum":"   11","line":"    return Unit{};","class":"lineCov","hits":"1","order":"2832",},
{"lineNum":"   12","line":"}","class":"lineCov","hits":"1","order":"2834",},
{"lineNum":"   13","line":""},
{"lineNum":"   14","line":"auto Analyzer::collect_symbols(mod::Module& module) -> void {","class":"lineCov","hits":"1","order":"2831",},
{"lineNum":"   15","line":"    Diagnostics diagnostics;","class":"lineCov","hits":"1","order":"2830",},
{"lineNum":"   16","line":"    SymbolCollector::collect_symbols(module, {modules_, registry_, pool_, diagnostics});","class":"lineCov","hits":"1","order":"2829",},
{"lineNum":"   17","line":"}","class":"lineCov","hits":"1","order":"2828",},
{"lineNum":"   18","line":""},
{"lineNum":"   19","line":"} // namespace porpoise::sema"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 9, "covered" : 9,};
var merged_data = [];

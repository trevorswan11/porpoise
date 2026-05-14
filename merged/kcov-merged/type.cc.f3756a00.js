var data = {lines:[
{"lineNum":"    1","line":"#include \"sema/type.hh\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"namespace porpoise::sema {"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"auto TypePool::get_opt(const types::Key& key) noexcept -> opt::Option<Type&> {","class":"lineCov","hits":"1","order":"1826",},
{"lineNum":"    6","line":"    if (auto it = cache_.find(key); it != cache_.end()) { return *it->second; }","class":"lineCov","hits":"1","order":"1824",},
{"lineNum":"    7","line":"    return opt::none;","class":"lineCov","hits":"1","order":"1823",},
{"lineNum":"    8","line":"}","class":"lineCov","hits":"1","order":"1822",},
{"lineNum":"    9","line":""},
{"lineNum":"   10","line":"auto TypePool::get_or_emplace(const types::Key& key) -> Type& {","class":"lineCov","hits":"1","order":"1825",},
{"lineNum":"   11","line":"    if (auto type = get_opt(key)) { return *type; }","class":"lineCov","hits":"1","order":"1821",},
{"lineNum":"   12","line":"    auto* type = arena_.make<Type>(key.get_kind()).get();","class":"lineCov","hits":"1","order":"1820",},
{"lineNum":"   13","line":"    cache_.emplace(key, type);","class":"lineCov","hits":"1","order":"1819",},
{"lineNum":"   14","line":"    return *type;","class":"lineCov","hits":"1","order":"1818",},
{"lineNum":"   15","line":"}","class":"lineCov","hits":"1","order":"1817",},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"} // namespace porpoise::sema"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-05-14 22:03:35", "instrumented" : 10, "covered" : 10,};
var merged_data = [];

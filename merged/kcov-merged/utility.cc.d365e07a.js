var data = {lines:[
{"lineNum":"    1","line":"#include \"utility.hh\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include <config.h>"},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"#if PLATFORM_WINDOWS"},
{"lineNum":"    6","line":"#    include <io.h>"},
{"lineNum":"    7","line":"#    define ISATTY _isatty"},
{"lineNum":"    8","line":"#    define STDOUT_FILENO 1"},
{"lineNum":"    9","line":"#else"},
{"lineNum":"   10","line":"#    include <unistd.h>"},
{"lineNum":"   11","line":"#    define ISATTY isatty"},
{"lineNum":"   12","line":"#endif"},
{"lineNum":"   13","line":""},
{"lineNum":"   14","line":"namespace porpoise {"},
{"lineNum":"   15","line":""},
{"lineNum":"   16","line":"auto is_tty() noexcept -> bool { return ISATTY(STDOUT_FILENO); }","class":"lineCov","hits":"1","order":"692",},
{"lineNum":"   17","line":""},
{"lineNum":"   18","line":"} // namespace porpoise"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-05-14 22:03:35", "instrumented" : 1, "covered" : 1,};
var merged_data = [];

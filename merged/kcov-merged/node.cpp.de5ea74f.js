var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/node.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"enum.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"constexpr auto NODE_NAMES = [] {"},
{"lineNum":"    8","line":"    EnumMap<NodeKind, std::string_view> names{\"expression\"};"},
{"lineNum":"    9","line":""},
{"lineNum":"   10","line":"    names[NodeKind::ENUM_EXPRESSION]     = \"enum\";"},
{"lineNum":"   11","line":"    names[NodeKind::FUNCTION_EXPRESSION] = \"function\";"},
{"lineNum":"   12","line":"    names[NodeKind::UNION_EXPRESSION]    = \"union\";"},
{"lineNum":"   13","line":"    names[NodeKind::STRUCT_EXPRESSION]   = \"struct\";"},
{"lineNum":"   14","line":""},
{"lineNum":"   15","line":"    for (const auto kind : enum_range<NodeKind::BLOCK_STATEMENT, NodeKind::USING_STATEMENT>()) {"},
{"lineNum":"   16","line":"        names[kind] = \"statement\";"},
{"lineNum":"   17","line":"    }"},
{"lineNum":"   18","line":"    return names;"},
{"lineNum":"   19","line":"}();"},
{"lineNum":"   20","line":""},
{"lineNum":"   21","line":"auto Node::display_name() const noexcept -> std::string_view { return NODE_NAMES[kind_]; }","class":"lineCov","hits":"1","order":"1476",},
{"lineNum":"   22","line":""},
{"lineNum":"   23","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 1, "covered" : 1,};
var merged_data = [];

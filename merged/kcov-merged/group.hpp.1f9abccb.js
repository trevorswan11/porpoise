var data = {lines:[
{"lineNum":"    1","line":"#pragma once"},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/node.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"#include \"syntax/parser.hpp\""},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"namespace porpoise::ast {"},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"class GroupedExpression {"},
{"lineNum":"   10","line":"  public:"},
{"lineNum":"   11","line":"    [[nodiscard]] static auto parse(syntax::Parser& parser)"},
{"lineNum":"   12","line":"        -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1755",},
{"lineNum":"   13","line":"        parser.advance();","class":"lineCov","hits":"1","order":"1753",},
{"lineNum":"   14","line":"        auto inner = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1752",},
{"lineNum":"   15","line":"        TRY(parser.expect_peek(syntax::TokenType::RPAREN));","class":"lineCov","hits":"1","order":"1751",},
{"lineNum":"   16","line":"        return inner;","class":"lineCov","hits":"1","order":"1750",},
{"lineNum":"   17","line":"    }","class":"lineCov","hits":"1","order":"1754",},
{"lineNum":"   18","line":"};"},
{"lineNum":"   19","line":""},
{"lineNum":"   20","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 6, "covered" : 6,};
var merged_data = [];

var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/identifier.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"2747",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IdentifierExpression::parse("},
{"lineNum":"   10","line":"    syntax::Parser& parser) // cppcheck-suppress constParameterReference"},
{"lineNum":"   11","line":"    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {","class":"lineCov","hits":"1","order":"2744",},
{"lineNum":"   12","line":"    const auto start_token = parser.get_current_token();","class":"lineCov","hits":"1","order":"2745",},
{"lineNum":"   13","line":"    if (!start_token.is_valid_ident()) {","class":"lineCov","hits":"1","order":"2743",},
{"lineNum":"   14","line":"        return make_parser_err(syntax::ParserError::ILLEGAL_IDENTIFIER, start_token);","class":"lineCov","hits":"1","order":"2742",},
{"lineNum":"   15","line":"    }"},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"    return mem::make_box<IdentifierExpression>(start_token);","class":"lineCov","hits":"1","order":"2741",},
{"lineNum":"   18","line":"}","class":"lineCov","hits":"1","order":"2746",},
{"lineNum":"   19","line":""},
{"lineNum":"   20","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 7, "covered" : 7,};
var merged_data = [];

var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/expressions/index.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"1939",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto IndexExpression::parse(syntax::Parser& parser, mem::Box<Expression> array)"},
{"lineNum":"   10","line":"    -> Result<mem::Box<Expression>, syntax::ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1936",},
{"lineNum":"   11","line":"    const auto start_token = array->get_token();","class":"lineCov","hits":"1","order":"1935",},
{"lineNum":"   12","line":"    if (parser.peek_token_is(syntax::TokenType::RBRACKET)) {","class":"lineCov","hits":"1","order":"1937",},
{"lineNum":"   13","line":"        return make_parser_err(syntax::ParserError::INDEX_MISSING_EXPRESSION, start_token);","class":"lineCov","hits":"1","order":"1934",},
{"lineNum":"   14","line":"    }"},
{"lineNum":"   15","line":"    parser.advance();","class":"lineCov","hits":"1","order":"1933",},
{"lineNum":"   16","line":""},
{"lineNum":"   17","line":"    auto idx_expr = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1932",},
{"lineNum":"   18","line":"    TRY(parser.expect_peek(syntax::TokenType::RBRACKET));","class":"lineCov","hits":"1","order":"1938",},
{"lineNum":"   19","line":"    return mem::make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));","class":"lineCov","hits":"1","order":"1930",},
{"lineNum":"   20","line":"}","class":"lineCov","hits":"1","order":"1931",},
{"lineNum":"   21","line":""},
{"lineNum":"   22","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 10, "covered" : 10,};
var merged_data = [];

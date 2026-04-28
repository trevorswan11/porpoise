var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/return.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto ReturnStatement::accept(Visitor& v) const noexcept -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"618",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto ReturnStatement::parse(syntax::Parser& parser)"},
{"lineNum":"   10","line":"    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {","class":"lineCov","hits":"1","order":"616",},
{"lineNum":"   11","line":"    const auto start_token = parser.get_current_token();","class":"lineCov","hits":"1","order":"615",},
{"lineNum":"   12","line":""},
{"lineNum":"   13","line":"    mem::NullableBox<Expression> value;","class":"lineCov","hits":"1","order":"613",},
{"lineNum":"   14","line":"    if (!parser.peek_token_is(syntax::TokenType::END) &&","class":"lineCov","hits":"1","order":"612",},
{"lineNum":"   15","line":"        !parser.peek_token_is(syntax::TokenType::SEMICOLON)) {","class":"lineCov","hits":"1","order":"611",},
{"lineNum":"   16","line":"        parser.advance();","class":"lineCov","hits":"1","order":"610",},
{"lineNum":"   17","line":"        value = mem::nullable_box_from(TRY(parser.parse_expression()));","class":"lineCov","hits":"1","order":"609",},
{"lineNum":"   18","line":"    }","class":"lineCov","hits":"1","order":"617",},
{"lineNum":"   19","line":""},
{"lineNum":"   20","line":"    TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));","class":"lineCov","hits":"1","order":"608",},
{"lineNum":"   21","line":"    return mem::make_box<ReturnStatement>(start_token, std::move(value));","class":"lineCov","hits":"1","order":"607",},
{"lineNum":"   22","line":"}","class":"lineCov","hits":"1","order":"614",},
{"lineNum":"   23","line":""},
{"lineNum":"   24","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 12, "covered" : 12,};
var merged_data = [];

var data = {lines:[
{"lineNum":"    1","line":"#include \"ast/statements/expression.hpp\""},
{"lineNum":"    2","line":""},
{"lineNum":"    3","line":"#include \"ast/visitor.hpp\""},
{"lineNum":"    4","line":""},
{"lineNum":"    5","line":"namespace porpoise::ast {"},
{"lineNum":"    6","line":""},
{"lineNum":"    7","line":"auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }","class":"lineCov","hits":"1","order":"1312",},
{"lineNum":"    8","line":""},
{"lineNum":"    9","line":"auto ExpressionStatement::parse(syntax::Parser& parser, [[maybe_unused]] bool require_semicolon)"},
{"lineNum":"   10","line":"    -> Result<mem::Box<Statement>, syntax::ParserDiagnostic> {","class":"lineCov","hits":"1","order":"1309",},
{"lineNum":"   11","line":"    const auto start_token = parser.get_current_token();","class":"lineCov","hits":"1","order":"1308",},
{"lineNum":"   12","line":"    auto       expr        = TRY(parser.parse_expression());","class":"lineCov","hits":"1","order":"1310",},
{"lineNum":"   13","line":""},
{"lineNum":"   14","line":"    if (!parser.current_token_is(syntax::TokenType::SEMICOLON)) {","class":"lineCov","hits":"1","order":"1306",},
{"lineNum":"   15","line":"        if (parser.peek_token_is(syntax::TokenType::SEMICOLON)) {","class":"lineCov","hits":"1","order":"1305",},
{"lineNum":"   16","line":"            parser.advance();","class":"lineCov","hits":"1","order":"1304",},
{"lineNum":"   17","line":"        } else if (require_semicolon) {","class":"lineCov","hits":"1","order":"1303",},
{"lineNum":"   18","line":"            TRY(parser.expect_peek(syntax::TokenType::SEMICOLON));","class":"lineCov","hits":"1","order":"1311",},
{"lineNum":"   19","line":"        }","class":"lineNoCov","hits":"0",},
{"lineNum":"   20","line":"    }","class":"lineCov","hits":"1","order":"1302",},
{"lineNum":"   21","line":"    return mem::make_box<ExpressionStatement>(start_token, std::move(expr));","class":"lineCov","hits":"1","order":"1301",},
{"lineNum":"   22","line":"}","class":"lineCov","hits":"1","order":"1307",},
{"lineNum":"   23","line":""},
{"lineNum":"   24","line":"} // namespace porpoise::ast"},
]};
var percent_low = 25;var percent_high = 75;
var header = { "command" : "", "date" : "2026-04-28 20:58:39", "instrumented" : 13, "covered" : 12,};
var merged_data = [];

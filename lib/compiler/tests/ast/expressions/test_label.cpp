#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

namespace porpoise::tests {

namespace keywords = syntax::keywords;

const syntax::Token a{syntax::TokenType::IDENT, "a"};

TEST_CASE("Labeled do-while") {
    helpers::test_expr_stmt(
        "a: do {b;} while (true);",
        ast::LabelExpression{a,
                             helpers::make_ident(a),
                             mem::make_box<ast::DoWhileLoopExpression>(
                                 syntax::Token{keywords::DO},
                                 helpers::make_expr_block_stmt(helpers::ident_from("b")),
                                 helpers::make_primitive<ast::BoolExpression>(true))});
}

TEST_CASE("Labeled for") {
    helpers::test_expr_stmt(
        "a: for (arr) |i| { b; };",
        ast::LabelExpression{
            a,
            helpers::make_ident(a),
            mem::make_box<ast::ForLoopExpression>(
                syntax::Token{keywords::FOR},
                helpers::make_vector<mem::Box<ast::Expression>>(helpers::make_ident("arr")),
                helpers::make_vector<ast::ForLoopCapture>(
                    ast::ForLoopCapture::Valued{{}, helpers::make_ident("i")}),
                helpers::make_expr_block_stmt(helpers::ident_from("b")),
                nullptr)});
}

TEST_CASE("Labeled if") {
    helpers::test_expr_stmt(
        "a: if (b) { c; } else { d; };",
        ast::LabelExpression{a,
                             helpers::make_ident(a),
                             mem::make_box<ast::IfExpression>(
                                 syntax::Token{keywords::IF},
                                 false,
                                 helpers::make_ident("b"),
                                 helpers::make_expr_block_stmt(helpers::ident_from("c")),
                                 helpers::make_expr_block_stmt<true>(helpers::ident_from("d")))});
}

TEST_CASE("Labeled infinite loop") {
    helpers::test_expr_stmt(
        "a: loop { b; };",
        ast::LabelExpression{a,
                             helpers::make_ident(a),
                             mem::make_box<ast::InfiniteLoopExpression>(
                                 syntax::Token{keywords::LOOP},
                                 helpers::make_expr_block_stmt(helpers::ident_from("b")))});
}

TEST_CASE("Labeled match") {
    helpers::test_expr_stmt(
        "a: match (b) { c => d; };",
        ast::LabelExpression{a,
                             helpers::make_ident(a),
                             mem::make_box<ast::MatchExpression>(
                                 syntax::Token{keywords::MATCH},
                                 helpers::make_ident("b"),
                                 helpers::make_vector<ast::MatchArm>(ast::MatchArm{
                                     helpers::make_ident("c"),
                                     {},
                                     helpers::make_expr_stmt(helpers::ident_from("d"))}),
                                 nullptr)});
}

TEST_CASE("Labeled while") {
    helpers::test_expr_stmt(
        "a: while (true) {b;};",
        ast::LabelExpression{a,
                             helpers::make_ident(a),
                             mem::make_box<ast::WhileLoopExpression>(
                                 syntax::Token{keywords::WHILE},
                                 helpers::make_primitive<ast::BoolExpression>(true),
                                 nullptr,
                                 helpers::make_expr_block_stmt(helpers::ident_from("b")),
                                 nullptr)});
}

TEST_CASE("Labeled block") {
    helpers::test_expr_stmt(
        "a: {b;};",
        ast::LabelExpression{
            a, helpers::make_ident(a), helpers::make_expr_block_stmt(helpers::ident_from("b"))});
}

TEST_CASE("Non-ident label") {
    helpers::test_parser_fail("2: {};",
                              syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL, 0, 0});
}

TEST_CASE("Illegal label expressions") {
    helpers::test_parser_fail(
        "a: 3;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail(
        "a: b;", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail(
        "a: b();", syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_EXPRESSION, 0, 3});

    helpers::test_parser_fail(
        "a: b: c: {};",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_EXPRESSION, 0, 6});
}

TEST_CASE("Illegal label statements") {
    helpers::test_parser_fail(
        "a: defer 3;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_STATEMENT, 0, 3});

    helpers::test_parser_fail(
        "a: return 3;",
        syntax::ParserDiagnostic{syntax::ParserError::ILLEGAL_LABEL_STATEMENT, 0, 3});
}

} // namespace porpoise::tests

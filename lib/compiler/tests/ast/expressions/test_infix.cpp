#include <span>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hpp"

#include "array.hpp"

namespace porpoise::tests {

namespace operators = syntax::operators;

namespace helpers {

template <ast::LeafNode N, ast::LeafNode L, ast::LeafNode R>
auto test_infix_expr(std::string_view input, L&& lhs, syntax::TokenType op, R&& rhs) -> void {
    syntax::Lexer l{input};
    const auto&   start_token = l.advance();
    test_expr_stmt(
        input,
        N{start_token, mem::make_box<L>(std::move(lhs)), op, mem::make_box<R>(std::move(rhs))});
}

template <ast::LeafNode N> auto test_infix_op_list(std::span<const syntax::Operator> ops) -> void {
    for (const auto& op : ops) {
        const auto input = fmt::format("a {} b;", op.first);
        helpers::test_infix_expr<N>(input, ident_from("a"), op.second, ident_from("b"));
    }
}

template <ast::LeafNode L, ast::LeafNode R>
void test_binary_expr(std::string_view input, L&& lhs, syntax::TokenType op, R&& rhs) {
    helpers::test_infix_expr<ast::BinaryExpression>(
        input, std::forward<L>(lhs), op, std::forward<R>(rhs));
}

void test_binary_expr(std::string_view        input,
                      const syntax::Token&    start_token,
                      ast::BinaryExpression&& expected) {
    test_expr_stmt(input, start_token, std::move(expected));
}

} // namespace helpers

TEST_CASE("Assignment expressions") {
    const auto ops = std::array{
        operators::ASSIGN,
        operators::PLUS_ASSIGN,
        operators::MINUS_ASSIGN,
        operators::STAR_ASSIGN,
        operators::SLASH_ASSIGN,
        operators::PERCENT_ASSIGN,
        operators::BW_AND_ASSIGN,
        operators::BW_OR_ASSIGN,
        operators::SHL_ASSIGN,
        operators::SHR_ASSIGN,
        operators::NOT_ASSIGN,
        operators::XOR_ASSIGN,
    };
    helpers::test_infix_op_list<ast::AssignmentExpression>(ops);
}

TEST_CASE("Assignment operator right associativity") {
    const auto test_assignment_assoc = [](std::pair<syntax::Operator, syntax::Operator> ops) {
        const auto input = fmt::format("a {} b {} c;", ops.first.first, ops.second.first);
        helpers::test_infix_expr<ast::AssignmentExpression>(
            input,
            helpers::ident_from("a"),
            ops.first.second,
            ast::AssignmentExpression{
                syntax::Token{syntax::TokenType::IDENT, "b"},
                helpers::make_ident("b"),
                ops.second.second,
                helpers::make_ident("c"),
            });
    };

    const std::array assignment_ops{
        operators::ASSIGN,
        operators::PLUS_ASSIGN,
        operators::MINUS_ASSIGN,
        operators::STAR_ASSIGN,
        operators::SLASH_ASSIGN,
        operators::PERCENT_ASSIGN,
        operators::BW_AND_ASSIGN,
        operators::BW_OR_ASSIGN,
        operators::SHL_ASSIGN,
        operators::SHR_ASSIGN,
        operators::NOT_ASSIGN,
        operators::XOR_ASSIGN,
    };

    SECTION("Same operator") {
        for (const auto& op : assignment_ops) { test_assignment_assoc({op, op}); }
    }

    SECTION("Combinations") {
        for (const auto& ops : array::combinations(assignment_ops)) { test_assignment_assoc(ops); }
    }
}

TEST_CASE("Binary expressions") {
    const auto ops = std::array{
        operators::PLUS,
        operators::MINUS,
        operators::STAR,
        operators::SLASH,
        operators::PERCENT,
        operators::LT,
        operators::LT_EQ,
        operators::GT,
        operators::GT_EQ,
        operators::EQ,
        operators::NEQ,
        operators::BOOLEAN_AND,
        operators::BOOLEAN_OR,
        operators::BW_AND,
        operators::BW_OR,
        operators::XOR,
        operators::SHR,
        operators::SHL,
    };
    helpers::test_infix_op_list<ast::BinaryExpression>(ops);
}

TEST_CASE("Miscellaneous infixes") {
    SECTION("Range expressions") {
        const auto ops = std::array{
            operators::DOT_DOT,
            operators::DOT_DOT_EQ,
        };
        helpers::test_infix_op_list<ast::RangeExpression>(ops);
    }

    SECTION("Dot expression") {
        const auto ops = std::array{operators::DOT};
        helpers::test_infix_op_list<ast::DotExpression>(ops);
    }
}

const syntax::Token a{syntax::TokenType::IDENT, "a"};
const syntax::Token b{syntax::TokenType::IDENT, "b"};
const syntax::Token c{syntax::TokenType::IDENT, "c"};

TEST_CASE("Numerical precedence") {
    SECTION("Add/sub and mul/div") {
        helpers::test_binary_expr(
            "a + b * c;",
            helpers::ident_from(a),
            syntax::TokenType::PLUS,
            ast::BinaryExpression{
                b, helpers::make_ident(b), syntax::TokenType::STAR, helpers::make_ident(c)});

        helpers::test_binary_expr(
            "a + b / c;",
            helpers::ident_from(a),
            syntax::TokenType::PLUS,
            ast::BinaryExpression{
                b, helpers::make_ident(b), syntax::TokenType::SLASH, helpers::make_ident(c)});

        helpers::test_binary_expr(
            "a * b - c;",
            ast::BinaryExpression{
                a, helpers::make_ident(a), syntax::TokenType::STAR, helpers::make_ident(b)},
            syntax::TokenType::MINUS,
            helpers::ident_from(c));

        helpers::test_binary_expr(
            "a + b - c;",
            ast::BinaryExpression{
                a, helpers::make_ident(a), syntax::TokenType::PLUS, helpers::make_ident(b)},
            syntax::TokenType::MINUS,
            helpers::ident_from(c));
    }

    SECTION("Grouping") {
        helpers::test_binary_expr(
            "(a + b) * c;",
            syntax::Token{syntax::TokenType::LPAREN, "("},
            ast::BinaryExpression{
                a,
                mem::make_box<ast::BinaryExpression>(
                    a, helpers::make_ident(a), syntax::TokenType::PLUS, helpers::make_ident(b)),
                syntax::TokenType::STAR,
                helpers::make_ident(c)});
    }
}

const syntax::Token d{syntax::TokenType::IDENT, "d"};

TEST_CASE("Bitwise operations") {
    helpers::test_binary_expr(
        "a & b | c;",
        ast::BinaryExpression{
            a, helpers::make_ident(a), syntax::TokenType::BW_AND, helpers::make_ident(b)},
        syntax::TokenType::BW_OR,
        helpers::ident_from(c));

    helpers::test_binary_expr(
        "a | b & c;",
        helpers::ident_from(a),
        syntax::TokenType::BW_OR,
        ast::BinaryExpression{
            b, helpers::make_ident(b), syntax::TokenType::BW_AND, helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b & c;",
        helpers::ident_from(a),
        syntax::TokenType::BW_OR,
        ast::BinaryExpression{
            b, helpers::make_ident(b), syntax::TokenType::BW_AND, helpers::make_ident(c)});

    helpers::test_binary_expr(
        "(a | b) & c;",
        syntax::Token{syntax::TokenType::LPAREN, "("},
        ast::BinaryExpression{
            a,
            mem::make_box<ast::BinaryExpression>(
                a, helpers::make_ident(a), syntax::TokenType::BW_OR, helpers::make_ident(b)),
            syntax::TokenType::BW_AND,
            helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b ^ c & d;",
        ast::BinaryExpression{
            a, helpers::make_ident(a), syntax::TokenType::BW_OR, helpers::make_ident(b)},
        syntax::TokenType::XOR,
        ast::BinaryExpression{
            c, helpers::make_ident(c), syntax::TokenType::BW_AND, helpers::make_ident(d)});
}

const syntax::Token e{syntax::TokenType::IDENT, "e"};

TEST_CASE("Boolean operations") {
    helpers::test_binary_expr(
        "a < b and c > d;",
        ast::BinaryExpression{
            a, helpers::make_ident(a), syntax::TokenType::LT, helpers::make_ident(b)},
        syntax::TokenType::BOOLEAN_AND,
        ast::BinaryExpression{
            c, helpers::make_ident(c), syntax::TokenType::GT, helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a <= b or c == d and e;",
        ast::BinaryExpression{
            a,
            mem::make_box<ast::BinaryExpression>(
                a, helpers::make_ident(a), syntax::TokenType::LT_EQ, helpers::make_ident(b)),
            syntax::TokenType::BOOLEAN_OR,
            mem::make_box<ast::BinaryExpression>(
                c, helpers::make_ident(c), syntax::TokenType::EQ, helpers::make_ident(d))},
        syntax::TokenType::BOOLEAN_AND,
        helpers::ident_from(e));
}

TEST_CASE("Prefix precedence") {
    const syntax::Token minus{operators::MINUS};
    helpers::test_binary_expr(
        "a + -b * c;",
        helpers::ident_from(a),
        syntax::TokenType::PLUS,
        ast::BinaryExpression{minus,
                              mem::make_box<ast::UnaryExpression>(minus, helpers::make_ident(b)),
                              syntax::TokenType::STAR,
                              helpers::make_ident(c)});

    helpers::test_binary_expr(
        "a | b & ~c;",
        helpers::ident_from(a),
        syntax::TokenType::BW_OR,
        ast::BinaryExpression{b,
                              helpers::make_ident(b),
                              syntax::TokenType::BW_AND,
                              mem::make_box<ast::UnaryExpression>(syntax::Token{operators::NOT},
                                                                  helpers::make_ident(c))});

    helpers::test_binary_expr(
        "a and !b;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_AND,
        ast::UnaryExpression{syntax::Token{operators::BANG}, helpers::make_ident(b)});
}

TEST_CASE("Call precedence") {
    helpers::test_binary_expr(
        "a and b() != c;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_AND,
        ast::BinaryExpression{b,
                              mem::make_box<ast::CallExpression>(
                                  b, helpers::make_ident(b), std::vector<ast::CallArgument>{}),
                              syntax::TokenType::NEQ,
                              helpers::make_ident(c)});
}

TEST_CASE("Index precedence") {
    helpers::test_binary_expr(
        "a or b[3uz] == !c;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_OR,
        ast::BinaryExpression{b,
                              mem::make_box<ast::IndexExpression>(
                                  b,
                                  helpers::make_ident(b),
                                  mem::make_box<ast::USizeIntegerExpression>(
                                      syntax::Token{syntax::TokenType::UZINT_10, "3uz"}, 3uz)),
                              syntax::TokenType::EQ,
                              mem::make_box<ast::UnaryExpression>(syntax::Token{operators::BANG},
                                                                  helpers::make_ident(c))});
}

TEST_CASE("Scope resolution precedence") {
    helpers::test_binary_expr(
        "a or b.c >= d;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            mem::make_box<ast::DotExpression>(
                b, helpers::make_ident(b), syntax::TokenType::DOT, helpers::make_ident(c)),
            syntax::TokenType::GT_EQ,
            helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a or b::c < d;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_OR,
        ast::BinaryExpression{b,
                              mem::make_box<ast::ScopeResolutionExpression>(
                                  b, helpers::make_ident(b), helpers::make_ident(c)),
                              syntax::TokenType::LT,
                              helpers::make_ident(d)});

    helpers::test_binary_expr(
        "a or b.c >= d;",
        helpers::ident_from(a),
        syntax::TokenType::BOOLEAN_OR,
        ast::BinaryExpression{
            b,
            mem::make_box<ast::DotExpression>(
                b, helpers::make_ident(b), syntax::TokenType::DOT, helpers::make_ident(c)),
            syntax::TokenType::GT_EQ,
            helpers::make_ident(d)});
}

TEST_CASE("Illegal infix node") {
    helpers::test_parser_fail(
        "a and import std;",
        syntax::ParserDiagnostic{"No prefix parse function for IMPORT(import) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 7uz}});
}

TEST_CASE("Non-terminated infix") {
    helpers::test_parser_fail(
        "a and;",
        syntax::ParserDiagnostic{"No prefix parse function for SEMICOLON(;) found",
                                 syntax::ParserError::MISSING_PREFIX_PARSER,
                                 std::pair{1uz, 6uz}});
    helpers::test_parser_fail(
        "a and", syntax::ParserDiagnostic{syntax::ParserError::INFIX_MISSING_RHS, 1, 3});
}

} // namespace porpoise::tests
